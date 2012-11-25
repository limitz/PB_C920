#include "c920_rtsp_server.h"

#define C920_RTSP_SERVER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_SERVER, C920RtspServerPrivate))


G_DEFINE_TYPE(C920RtspServer, c920_rtsp_server, GST_TYPE_RTSP_SERVER);
 
struct _C920RtspServerPrivate
{
	C920RtspMediaMapping *mapping;
};

static void 	c920_rtsp_server_class_init(C920RtspServerClass*);
static void 	c920_rtsp_server_init(C920RtspServer*);
static void 	c920_rtsp_server_dispose(GObject*);
static void 	c920_rtsp_server_finalize(GObject*);
static gboolean c920_rtsp_server_timeout(C920RtspServer*, gboolean);


static void c920_rtsp_server_class_init(C920RtspServerClass *cls)
{
	GObjectClass *base = G_OBJECT_CLASS(cls);

	base->dispose  = c920_rtsp_server_dispose;
	base->finalize = c920_rtsp_server_finalize;

	g_type_class_add_private(cls, sizeof(C920RtspServerPrivate));
}

static void c920_rtsp_server_init(C920RtspServer *self)
{
	INFO("Initializing C920 RTSP Server");
	self->priv = C920_RTSP_SERVER_GET_PRIVATE(self);
	self->priv->mapping = g_object_new(C920_TYPE_RTSP_MEDIA_MAPPING, NULL);

	GstRTSPServer* base = GST_RTSP_SERVER(self);
	gst_rtsp_server_set_address(base, C920_RTSP_HOST);
	gst_rtsp_server_set_service(base, C920_RTSP_PORT);
	gst_rtsp_server_set_media_mapping(base, GST_RTSP_MEDIA_MAPPING(self->priv->mapping));

	INFO("Starting C920 RTSP Server on %s:%s", C920_RTSP_HOST, C920_RTSP_PORT);
	if (gst_rtsp_server_attach(base, NULL) == 0) g_critical("Unable to start RTSP server");
	INFO("Started RTSP server on %s:%s",C920_RTSP_HOST, C920_RTSP_PORT);

	g_timeout_add_seconds(2, (GSourceFunc)c920_rtsp_server_timeout, self);
}

static void c920_rtsp_server_dispose(GObject *obj)
{
	C920RtspServer *self = C920_RTSP_SERVER(obj);
	g_object_unref(self->priv->mapping);
	G_OBJECT_CLASS(c920_rtsp_server_parent_class)->dispose(obj);
}

static void c920_rtsp_server_finalize(GObject* obj)
{
	C920RtspServer *self __unused = C920_RTSP_SERVER(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_server_parent_class)->finalize(obj);
}

static gboolean c920_rtsp_server_timeout(C920RtspServer *self, gboolean ignored)
{
	GstRTSPSessionPool *pool = gst_rtsp_server_get_session_pool(GST_RTSP_SERVER(self));
	gst_rtsp_session_pool_cleanup(pool);
	g_object_unref(pool);
	return TRUE;
}

int c920_rtsp_server_attach(C920RtspServer *self)
{
	return gst_rtsp_server_attach(GST_RTSP_SERVER(self), NULL);
}

void c920_rtsp_server_set_address(C920RtspServer *self, const gchar* address)
{
	gst_rtsp_server_set_address(GST_RTSP_SERVER(self), address);
}

const gchar* c920_rtsp_server_get_address(C920RtspServer *self)
{
	return gst_rtsp_server_get_address(GST_RTSP_SERVER(self));
}

void c920_rtsp_server_set_service(C920RtspServer *self, const gchar* service)
{
	gst_rtsp_server_set_service(GST_RTSP_SERVER(self), service);
}

const gchar* c920_rtsp_server_get_service(C920RtspServer *self)
{
	return gst_rtsp_server_get_service(GST_RTSP_SERVER(self));
}


