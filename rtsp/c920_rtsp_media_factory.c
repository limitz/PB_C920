#include "c920_rtsp_media_factory.h"

#define C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactoryPrivate))


G_DEFINE_TYPE(C920RtspMediaFactory, c920_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);


struct _C920RtspMediaFactoryPrivate
{
	C920LiveSrc* live_src;
};

static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass*);
static void c920_rtsp_media_factory_init(C920RtspMediaFactory*);
static void c920_rtsp_media_factory_dispose(GObject*);
static void c920_rtsp_media_factory_finalize(GObject*);

static GstRTSPMedia* 	c920_rtsp_media_factory_real_construct(GstRTSPMediaFactory*, const GstRTSPUrl*);
static GstElement*   	c920_rtsp_media_factory_real_create_pipeline(GstRTSPMediaFactory*, GstRTSPMedia*);
static void		c920_rtsp_media_factory_real_configure(GstRTSPMediaFactory*, GstRTSPMedia*);
static GstElement*	c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory*, const GstRTSPUrl*);
static gchar*		c920_rtsp_media_factory_real_gen_key(GstRTSPMediaFactory*, const GstRTSPUrl*);

static gboolean		c920_rtsp_media_factory_bus_watch(GstBus*, GstMessage*, gpointer);

static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass *cls)
{
	GstRTSPMediaFactoryClass* base = GST_RTSP_MEDIA_FACTORY_CLASS(cls);

	base->construct 	= c920_rtsp_media_factory_real_construct;
	base->create_pipeline 	= c920_rtsp_media_factory_real_create_pipeline;
	base->configure		= c920_rtsp_media_factory_real_configure;
	base->get_element	= c920_rtsp_media_factory_real_get_element;
	base->gen_key		= c920_rtsp_media_factory_real_gen_key;

	G_OBJECT_CLASS(cls)->dispose = c920_rtsp_media_factory_dispose;
	G_OBJECT_CLASS(cls)->finalize = c920_rtsp_media_factory_finalize;

	g_type_class_add_private(cls, sizeof(C920RtspMediaFactoryPrivate));
}

static void c920_rtsp_media_factory_init(C920RtspMediaFactory *self)
{
	self->priv = C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(self);
	gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(self), TRUE);
}

static void c920_rtsp_media_factory_dispose(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	g_object_unref(self->priv->live_src);
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->dispose(obj);
}

static void c920_rtsp_media_factory_finalize(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->finalize(obj);
}

static GstRTSPMedia* c920_rtsp_media_factory_real_construct(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->construct(base, url);
}

static GstElement* c920_rtsp_media_factory_real_create_pipeline(GstRTSPMediaFactory *base, GstRTSPMedia *media)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->create_pipeline(base, media);
}

static void c920_rtsp_media_factory_real_configure(GstRTSPMediaFactory *base, GstRTSPMedia *media)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->configure(base, media);
}

static GstElement* c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// Create a live src
	if (!C920_IS_RTSP_MEDIA_FACTORY(base)) return NULL;
	
	GstElement *pipeline, *h264parse, *rtph264pay, *live_src;
	GstBus *bus;
	gchar *device_name;
	C920RtspMediaFactory* self = C920_RTSP_MEDIA_FACTORY(base);

	if (!(pipeline = gst_pipeline_new("c920_stream")))
		WARNING_RETURN(NULL, "Unable to create pipeline");

	if (!(h264parse = gst_element_factory_make("h264parse", "parser")))
	{
		gst_object_unref(pipeline);
		WARNING_RETURN(NULL, "Unable to create element h264parse");
	}

	if (!(rtph264pay = gst_element_factory_make("rtph264pay", "pay0")))
	{
		gst_object_unref(pipeline);
		gst_object_unref(h264parse);
		WARNING_RETURN(NULL, "Unable to create element rtph264pay");
	}

	device_name = g_strdup_printf("/dev%s", url->abspath);

	live_src = GST_ELEMENT(
		g_object_new(
			C920_TYPE_LIVE_SRC, 
			"device-name", device_name,
			NULL));

	g_free(device_name);	
	
	if (!live_src)
	{
		gst_object_unref(pipeline);
		gst_object_unref(h264parse);
		gst_object_unref(rtph264pay);
		WARNING_RETURN(NULL, "Unable to create live source");
	}

	g_object_set(G_OBJECT(rtph264pay), "pt", 96, NULL);
	
	if ((bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline))))
	{
		gst_bus_add_watch(bus, c920_rtsp_media_factory_bus_watch, self);
		gst_object_unref(bus);
	}
	else g_warning("Unable to get a bus for the pipeline");

	gst_bin_add_many(GST_BIN(pipeline), GST_ELEMENT(self->priv->live_src), h264parse, rtph264pay, NULL);
	gst_element_link_many(GST_ELEMENT(self->priv->live_src), h264parse, rtph264pay, NULL);

	return GST_ELEMENT(pipeline);

	//return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->get_element(base, url);
}

static gchar* c920_rtsp_media_factory_real_gen_key(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// todo: add own logid
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->gen_key(base, url);
}

static gboolean	c920_rtsp_media_factory_bus_watch(GstBus *bus, GstMessage *msg, gpointer userdata)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(userdata);

	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS: break;
		case GST_MESSAGE_ERROR: break;
		default: break;
	}
	return TRUE;
}
