#include "c920_live_src.h"

#define C920_LIVE_SRC_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_LIVE_SRC, C920LiveSrcPrivate))


G_DEFINE_TYPE(C920LiveSrc, c920_live_src, GST_TYPE_APP_SRC);


struct _C920LiveSrcPrivate
{
	GstAppSrcCallbacks cb;
	GstCaps *caps;
};

static void c920_live_src_class_init(C920LiveSrcClass*);
static void c920_live_src_init(C920LiveSrc*);
static void c920_live_src_dispose(GObject*);
static void c920_live_src_finalize(GObject*);

static void c920_live_src_need_data(GstAppSrc*, guint, gpointer);
static void c920_live_src_enough_data(GstAppSrc*, gpointer);

static void c920_live_src_class_init(C920LiveSrcClass *cls)
{
	G_OBJECT_CLASS(cls)->dispose = c920_live_src_dispose;
	G_OBJECT_CLASS(cls)->finalize = c920_live_src_finalize;
	g_type_class_add_private(cls, sizeof(C920LiveSrcPrivate));
}

static void c920_live_src_init(C920LiveSrc *self)
{
	GstAppSrc* base = GST_APP_SRC(self);

	self->priv = C920_LIVE_SRC_GET_PRIVATE(self);

	self->priv->caps = gst_caps_new_simple(
				"video/x-h264", 
				"width", G_TYPE_INT, 1280, 
				"height", G_TYPE_INT,  720,
				"framerate", GST_TYPE_FRACTION, 30, 1,
				NULL);

	self->priv->cb.need_data = c920_live_src_need_data;
	self->priv->cb.enough_data = c920_live_src_enough_data;
	self->priv->cb.seek_data = NULL; // not implemented for live stream at this point

	gst_app_src_set_latency(base, -1, 1000);
	gst_app_src_set_size(base, -1);
	gst_app_src_set_stream_type(base, GST_APP_STREAM_TYPE_STREAM);
	gst_app_src_set_max_bytes(base, 20 * 1024 * 1024);
	gst_app_src_set_caps(base, self->priv->caps);
	gst_app_src_set_callbacks(base, &self->priv->cb, NULL, NULL);

	// todo start proc that starts the cam
}

static void c920_live_src_dispose(GObject *obj)
{
	C920LiveSrc* self __unused = C920_LIVE_SRC(obj);
	// todo ??? g_object_unref(self->priv->caps);
	G_OBJECT_CLASS(c920_live_src_parent_class)->dispose(obj);
}

static void c920_live_src_finalize(GObject *obj)
{
	C920LiveSrc* self __unused = C920_LIVE_SRC(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_live_src_parent_class)->finalize(obj);
}

static void c920_live_src_need_data(GstAppSrc *appsrc, guint length, gpointer userdata)
{
	C920LiveSrc *self __unused = C920_LIVE_SRC(appsrc);
	/* todo: implement logic */
}

static void c920_live_src_enough_data(GstAppSrc *appsrc, gpointer userdata)
{
	C920LiveSrc *self __unused = C920_LIVE_SRC(appsrc);
	/* todo: implement logic */
}
