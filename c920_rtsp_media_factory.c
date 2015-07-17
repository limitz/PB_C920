#include "c920_rtsp_media_factory.h"

G_DEFINE_TYPE(C920RtspMediaFactory, c920_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);

#define C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactoryPrivate))

struct _C920RtspMediaFactoryPrivate
{
	C920LiveSrc *live_src;
};

/* Forward declarations */
static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass*);
static void c920_rtsp_media_factory_init(C920RtspMediaFactory*);
static void c920_rtsp_media_factory_dispose(GObject*);
static void c920_rtsp_media_factory_finalize(GObject*);

/* GstRTSPMediaFactory overloads */
static GstElement* c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory*, const GstRTSPUrl*);

/* Utility functions */
static gboolean	c920_rtsp_media_factory_bus_watch(GstBus*, GstMessage*, gpointer);


/* C920 RTSP Media Factory Class Initializer 
 * -----------------------------------------
 * Overload the GstRTSPMediaFactory protected functions
 */
static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);
	GstRTSPMediaFactoryClass *base_class = GST_RTSP_MEDIA_FACTORY_CLASS(cls);

	base_class->get_element     = c920_rtsp_media_factory_real_get_element;

	object_class->dispose       = c920_rtsp_media_factory_dispose;
	object_class->finalize      = c920_rtsp_media_factory_finalize;

	g_type_class_add_private(cls, sizeof(C920RtspMediaFactoryPrivate));
}


/* C920 RTSP Media Factory Instance Initializer
 * --------------------------------------------
 * Sets the media factory to shared
 */
static void c920_rtsp_media_factory_init(C920RtspMediaFactory *self)
{
	self->priv = C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(self);
	gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(self), TRUE);
}


/* C920 RTSP Media Factory Instance Dispose
 * ----------------------------------------
 * Unref the live source?
 */
static void c920_rtsp_media_factory_dispose(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	// todo, check if this isn't already unreffed with the pipeline
	//g_object_unref(self->priv->live_src);
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->dispose(obj);
}


/* C920 RTSP Media Factory Instance Finalize
 * -----------------------------------------
 * Nothing to do for now
 */
static void c920_rtsp_media_factory_finalize(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->finalize(obj);
}


/* C920 RTSP Media Factory - Get Element
 * -------------------------------------
 * Create a pipeline with a live source streaming H.264 into a RTP payloader
 */
static GstElement* c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	if (!C920_IS_RTSP_MEDIA_FACTORY(base)) return NULL;
	
	GstElement *pipeline, *h264parse, *rtph264pay, *queue, *live_src;
	GstBus *bus;
	gchar *device_name;
	C920RtspMediaFactory *self = C920_RTSP_MEDIA_FACTORY(base);

	if (!(pipeline = gst_pipeline_new("c920_stream")))
		WARNING_RETURN(NULL, "Unable to create pipeline");

	if (!(h264parse = gst_element_factory_make("h264parse", "parser")))
	{
		gst_object_unref(pipeline);
		WARNING_RETURN(NULL, "Unable to create element h264parse");
	}

	if (!(queue = gst_element_factory_make("queue", "queue")))
	{
		gst_object_unref(pipeline);
		gst_object_unref(queue);
		WARNING_RETURN(NULL, "Unable to create element queue");
	}

	if (!(rtph264pay = gst_element_factory_make("rtph264pay", "pay0")))
	{
		gst_object_unref(pipeline);
		gst_object_unref(h264parse);
		gst_object_unref(queue);
		WARNING_RETURN(NULL, "Unable to create element rtph264pay");
	}

	device_name = g_strdup_printf("/dev%s", url->abspath);

	live_src = GST_ELEMENT(
		g_object_new(
			C920_TYPE_LIVE_SRC, 
			"device-name", device_name,
			NULL));

	g_free(device_name);	
	
	if (!live_src || !c920_live_src_validate(live_src))
	{
		gst_object_unref(pipeline);
		gst_object_unref(h264parse);
		gst_object_unref(queue);
		gst_object_unref(rtph264pay);
		WARNING_RETURN(NULL, "Unable to create live source");
	}

	self->priv->live_src = C920_LIVE_SRC(live_src);

	g_object_set(G_OBJECT(rtph264pay), "pt", 96, NULL);
	
	if ((bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline))))
	{
		gst_bus_add_watch(bus, c920_rtsp_media_factory_bus_watch, self);
		gst_object_unref(bus);
	}
	else g_warning("Unable to get a bus for the pipeline");

	gst_bin_add_many(GST_BIN(pipeline), GST_ELEMENT(self->priv->live_src), h264parse, queue, rtph264pay, NULL);
	gst_element_link_many(GST_ELEMENT(self->priv->live_src), h264parse, queue, rtph264pay, NULL);

	return GST_ELEMENT(pipeline);

	//return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->get_element(base, url);
}


static gboolean	c920_rtsp_media_factory_bus_watch(GstBus *bus, GstMessage *msg, gpointer userdata)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(userdata);

	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS: 
		{
			g_print("Bus message: EOS\n");
			break;
		}
		case GST_MESSAGE_ERROR:
		{ 
			GError *err = NULL;
			gchar *dbg_info = NULL;

			gst_message_parse_error (msg, &err, &dbg_info);
			g_printerr ("ERROR from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
			g_error_free (err);
			g_free (dbg_info);
			break;
		}
		default: 
			g_print("%d\n",GST_MESSAGE_TYPE(msg));
			break;
	}
	return TRUE;
}
