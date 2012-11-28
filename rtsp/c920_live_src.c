#include "c920_live_src.h"

#define C920_LIVE_SRC_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_LIVE_SRC, C920LiveSrcPrivate))


G_DEFINE_TYPE(C920LiveSrc, c920_live_src, GST_TYPE_APP_SRC);

enum
{
	PROP_0,
	PROP_DEVICE_NAME,
	N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {0};

struct _C920LiveSrcPrivate
{
	GstAppSrcCallbacks cb;
	GstCaps *caps;
	gboolean queue_full;

	C920VideoDevice *device;
	gchar *device_name;
};

static void c920_live_src_class_init(C920LiveSrcClass*);
static void c920_live_src_init(C920LiveSrc*);
static void c920_live_src_get_property(GObject*, guint, GValue*, GParamSpec*);
static void c920_live_src_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void c920_live_src_dispose(GObject*);
static void c920_live_src_finalize(GObject*);

static void c920_live_src_set_device_name(C920LiveSrc*, const gchar*);

static void c920_live_src_handle_buffer(gconstpointer, guint, gpointer);
static void c920_live_src_need_data(GstAppSrc*, guint, gpointer);
static void c920_live_src_enough_data(GstAppSrc*, gpointer);

static void c920_live_src_class_init(C920LiveSrcClass *cls)
{
	GObjectClass *object_cls = G_OBJECT_CLASS(cls);

	object_cls->get_property = c920_live_src_get_property;
	object_cls->set_property = c920_live_src_set_property;
	object_cls->dispose = c920_live_src_dispose;
	object_cls->finalize = c920_live_src_finalize;

	obj_properties[PROP_DEVICE_NAME] = 
		g_param_spec_string(
			"device-name",
			"Device name",
			"Set the device name",
			"",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	g_object_class_install_properties(object_cls, N_PROPERTIES, obj_properties);
	g_type_class_add_private(cls, sizeof(C920LiveSrcPrivate));
}

static void c920_live_src_init(C920LiveSrc *self)
{
	GstAppSrc* base = GST_APP_SRC(self);

	self->priv = C920_LIVE_SRC_GET_PRIVATE(self);

	self->priv->cb.need_data = c920_live_src_need_data;
	self->priv->cb.enough_data = c920_live_src_enough_data;
	self->priv->cb.seek_data = NULL; // not implemented for live stream at this point

	gst_app_src_set_latency(base, -1, 1000);
	gst_app_src_set_size(base, -1);
	gst_app_src_set_stream_type(base, GST_APP_STREAM_TYPE_STREAM);
	gst_app_src_set_max_bytes(base, 20 * 1024 * 1024);
	gst_app_src_set_callbacks(base, &self->priv->cb, NULL, NULL);

	// todo start proc that starts the cam
}

static void c920_live_src_get_property(GObject *obj, guint pidx, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(C920_IS_LIVE_SRC(obj));
	C920LiveSrc *self = C920_LIVE_SRC(obj);
	
	switch (pidx)
	{
		case PROP_DEVICE_NAME:
			g_value_set_string(value, self->priv->device_name);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, pidx, pspec);
			break;
	}
}

static void c920_live_src_set_property(GObject *obj, guint pidx, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(C920_IS_LIVE_SRC(obj));
	C920LiveSrc *self = C920_LIVE_SRC(obj);

	switch (pidx)
	{
		case PROP_DEVICE_NAME:
			c920_live_src_set_device_name(self, g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, pidx, pspec);
			break;
	}
}

static void c920_live_src_dispose(GObject *obj)
{
	C920LiveSrc *self = C920_LIVE_SRC(obj);
	gst_caps_unref(self->priv->caps);
	if (self->priv->device)
	{
		c920_video_device_remove_callback_by_data(self->priv->device, self);
		g_object_unref(self->priv->device);
	}
	G_OBJECT_CLASS(c920_live_src_parent_class)->dispose(obj);
}

static void c920_live_src_finalize(GObject *obj)
{
	C920LiveSrc *self = C920_LIVE_SRC(obj);
	g_free(self->priv->device_name);
	G_OBJECT_CLASS(c920_live_src_parent_class)->finalize(obj);
}

static void c920_live_src_set_device_name(C920LiveSrc *self, const gchar *device_name)
{
	g_return_if_fail(C920_IS_LIVE_SRC(self));
	g_free(self->priv->device_name);
	self->priv->device_name = g_strdup(device_name);

	C920DeviceManager *instance = g_object_new(C920_TYPE_DEVICE_MANAGER, NULL);
	self->priv->device = c920_device_manager_get_device(instance, device_name);

	c920_video_device_add_callback(self->priv->device, c920_live_src_handle_buffer, self);

	g_return_if_fail(self->priv->device != NULL);
	self->priv->caps = gst_caps_new_simple(
		"video/x-h264",
		"width", G_TYPE_INT, c920_video_device_get_width(self->priv->device),
		"height", G_TYPE_INT, c920_video_device_get_height(self->priv->device),
		"framerate", GST_TYPE_FRACTION, c920_video_device_get_fps(self->priv->device), 1,
		NULL);

	gst_app_src_set_caps(GST_APP_SRC(self), self->priv->caps);
}

static void c920_live_src_handle_buffer(gconstpointer data, guint length, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(userdata));
	C920LiveSrc *self = C920_LIVE_SRC(userdata);

	if (!self->priv->queue_full)
	{
		// todo: do we need to copy? i think yes
		GstBuffer *buffer = gst_buffer_new();
		if (buffer)
		{
			buffer->size = length;
			buffer->malloc_data = buffer->data = g_malloc(length);
			memcpy(buffer->malloc_data, data, length);
			gst_app_src_push_buffer(GST_APP_SRC(self), buffer);
		}
	}
}

static void c920_live_src_need_data(GstAppSrc *appsrc, guint length, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(appsrc));

	C920LiveSrc *self = C920_LIVE_SRC(appsrc);
	self->priv->queue_full = FALSE;
}

static void c920_live_src_enough_data(GstAppSrc *appsrc, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(appsrc));
	C920LiveSrc *self = C920_LIVE_SRC(appsrc);
	self->priv->queue_full = TRUE;
}
