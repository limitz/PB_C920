#include "c920_live_src.h"

G_DEFINE_TYPE(C920LiveSrc, c920_live_src, GST_TYPE_APP_SRC);

#define C920_LIVE_SRC_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_LIVE_SRC, C920LiveSrcPrivate))
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

/* Forward declarations */
static void c920_live_src_class_init(C920LiveSrcClass*);
static void c920_live_src_init(C920LiveSrc*);
static void c920_live_src_get_property(GObject*, guint, GValue*, GParamSpec*);
static void c920_live_src_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void c920_live_src_dispose(GObject*);
static void c920_live_src_finalize(GObject*);

/* Constructor only property setters */
static void c920_live_src_set_device_name(C920LiveSrc*, const gchar*);
static void c920_live_src_handle_buffer(gconstpointer, guint, gpointer);
static void c920_live_src_need_data(GstAppSrc*, guint, gpointer);
static void c920_live_src_enough_data(GstAppSrc*, gpointer);


/* C920 Live Src Class Initializer
 * -------------------------------
 * Set property declarations
 */
static void c920_live_src_class_init(C920LiveSrcClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);

	object_class->get_property = c920_live_src_get_property;
	object_class->set_property = c920_live_src_set_property;
	object_class->dispose      = c920_live_src_dispose;
	object_class->finalize     = c920_live_src_finalize;

	obj_properties[PROP_DEVICE_NAME] = 
		g_param_spec_string(
			"device-name",
			"Device name",
			"Set the device name",
			"",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
	g_type_class_add_private(cls, sizeof(C920LiveSrcPrivate));
}


/* C920 Live Src Instance Initializer
 * ----------------------------------
 * Set AppSrc Callbacks and initialize default settings
 */
static void c920_live_src_init(C920LiveSrc *self)
{
	GstAppSrc *base = GST_APP_SRC(self);

	self->priv = C920_LIVE_SRC_GET_PRIVATE(self);

	self->priv->cb.need_data = c920_live_src_need_data;
	self->priv->cb.enough_data = c920_live_src_enough_data;
	self->priv->cb.seek_data = NULL;

	gst_app_src_set_latency(base, 100, 1000);
	gst_app_src_set_size(base, -1);
	gst_app_src_set_stream_type(base, GST_APP_STREAM_TYPE_STREAM);
	gst_app_src_set_max_bytes(base, 2 * 1024 * 1024);
	gst_app_src_set_callbacks(base, &self->priv->cb, NULL, NULL);
}


/* C920 Live Src Property Getter
 * -----------------------------
 * For getting the device name
 */
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


/* C920 Live Src Property Setter
 * -----------------------------
 * For setting the device name
 */
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

/* C920 Live Src Dispose
 * ---------------------
 * Unref the caps.
 * Remove the callback, stop the device and unref the device.
 */
static void c920_live_src_dispose(GObject *obj)
{
	C920LiveSrc *self = C920_LIVE_SRC(obj);
	gst_caps_unref(self->priv->caps);
	if (self->priv->device)
	{
		c920_video_device_stop(self->priv->device);
		c920_video_device_remove_callback_by_data(self->priv->device, self);
		g_object_unref(self->priv->device);
	}
	G_OBJECT_CLASS(c920_live_src_parent_class)->dispose(obj);
}


/* C920 Live Src Finalize
 * ----------------------
 * Free the device name string
 */
static void c920_live_src_finalize(GObject *obj)
{
	C920LiveSrc *self = C920_LIVE_SRC(obj);
	g_free(self->priv->device_name);
	G_OBJECT_CLASS(c920_live_src_parent_class)->finalize(obj);
}


/* C920 Live Src - Set the device name
 * -----------------------------------
 * This will initialize and start the device. Can only be called from the constructor (once)
 */
static void c920_live_src_set_device_name(C920LiveSrc *self, const gchar *device_name)
{
	g_return_if_fail(C920_IS_LIVE_SRC(self));

	self->priv->device_name = g_strdup(device_name);

	// Create a singleton instance
	C920DeviceManager *instance = g_object_new(C920_TYPE_DEVICE_MANAGER, NULL);
	self->priv->device = c920_device_manager_get_device(instance, device_name);
	if (self->priv->device)
	{
		c920_video_device_add_callback(self->priv->device, c920_live_src_handle_buffer, self);
		c920_video_device_start(self->priv->device);
		g_object_unref(instance);

		self->priv->caps = gst_caps_new_simple(
			"video/x-raw",
			"width", G_TYPE_INT, c920_video_device_get_width(self->priv->device),
			"height", G_TYPE_INT, c920_video_device_get_height(self->priv->device),
			"framerate", GST_TYPE_FRACTION, c920_video_device_get_fps(self->priv->device), 1,
			NULL);
		//gst_app_src_set_caps(GST_APP_SRC(self), self->priv->caps);
	}
}


/* C920 Live Src - Validate
 * ------------------------
 * Check if the device has been loaded properly
 */
gboolean c920_live_src_validate(C920LiveSrc *self)
{
	return self->priv->device != NULL;
}


/* C920 Live Src - Handle a H.264 Buffer
 * -------------------------------------
 * Callback that's called from the device. Use the data and length to create a GstBuffer
 */
static void c920_live_src_handle_buffer(gconstpointer data, guint length, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(userdata));

	C920LiveSrc *self = C920_LIVE_SRC(userdata);
	if (!self->priv->queue_full)
	{
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


/* C920 Live Src - Need Data Callback
 * ----------------------------------
 * Handle the signal that the queue is empty
 */
static void c920_live_src_need_data(GstAppSrc *appsrc, guint length, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(appsrc));

	C920LiveSrc *self = C920_LIVE_SRC(appsrc);
	self->priv->queue_full = FALSE;
	//g_print("NEED DATA\n");
}


/* C920 Live Src - Enough Data Callback
 * ------------------------------------
 * Handle the signal that the queue is full
 */
static void c920_live_src_enough_data(GstAppSrc *appsrc, gpointer userdata)
{
	g_return_if_fail(C920_IS_LIVE_SRC(appsrc));
	C920LiveSrc *self = C920_LIVE_SRC(appsrc);
	g_print("BUFFER FULL\n");
	self->priv->queue_full = TRUE;
}
