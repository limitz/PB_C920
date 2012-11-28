#include "c920_rtsp_server.h"

G_DEFINE_TYPE(C920RtspServer, c920_rtsp_server, GST_TYPE_RTSP_SERVER);

#define C920_RTSP_SERVER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_SERVER, C920RtspServerPrivate))
 
struct _C920RtspServerPrivate
{
	C920RtspMediaMapping *mapping;
};

/* Forward Declarations */
static void 	c920_rtsp_server_class_init(C920RtspServerClass*);
static void 	c920_rtsp_server_init(C920RtspServer*);
static void 	c920_rtsp_server_dispose(GObject*);
static void 	c920_rtsp_server_finalize(GObject*);

/* Utility Function */
static gboolean c920_rtsp_server_timeout(C920RtspServer*, gboolean);


/* C920 RTSP Server Class Initializer
 * ----------------------------------
 * Nothing special here
 */
static void c920_rtsp_server_class_init(C920RtspServerClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);
	object_class->dispose      = c920_rtsp_server_dispose;
	object_class->finalize    = c920_rtsp_server_finalize;

	g_type_class_add_private(cls, sizeof(C920RtspServerPrivate));
}


/* C920 RTSP Server Instance Initializer
 * -------------------------------------
 * Add a new C920 Media Mapping
 */
static void c920_rtsp_server_init(C920RtspServer *self)
{	
	GstRTSPServer *base = GST_RTSP_SERVER(self);

	self->priv = C920_RTSP_SERVER_GET_PRIVATE(self);
	self->priv->mapping = g_object_new(C920_TYPE_RTSP_MEDIA_MAPPING, NULL);
	gst_rtsp_server_set_media_mapping(base, GST_RTSP_MEDIA_MAPPING(self->priv->mapping));
}


/* C920 RTSP Server Dispose
 * ------------------------
 * Unref the media mapping
 */
static void c920_rtsp_server_dispose(GObject *obj)
{
	C920RtspServer *self = C920_RTSP_SERVER(obj);
	g_object_unref(self->priv->mapping);
	G_OBJECT_CLASS(c920_rtsp_server_parent_class)->dispose(obj);
}


/* C920 RTSP Server Finalize
 * -------------------------
 * Nothing to do for now
 */
static void c920_rtsp_server_finalize(GObject* obj)
{
	C920RtspServer *self __unused = C920_RTSP_SERVER(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_server_parent_class)->finalize(obj);
}


/* C920 RTSP Server Timeout
 * ------------------------
 * A timeout function that cleans the pool every 2 seconds
 */
static gboolean c920_rtsp_server_timeout(C920RtspServer *self, gboolean ignored)
{
	GstRTSPSessionPool *pool = gst_rtsp_server_get_session_pool(GST_RTSP_SERVER(self));
	gst_rtsp_session_pool_cleanup(pool);
	g_object_unref(pool);
	return TRUE;
}


/* C920 RTSP Server Attach
 * -----------------------
 * Attach the server to the specified address and service
 */
int c920_rtsp_server_attach(C920RtspServer *self)
{
	int r = gst_rtsp_server_attach(GST_RTSP_SERVER(self), NULL);
	if (r == 0) 
		g_critical("Unable to start RTSP server on %s:%s", c920_rtsp_server_get_address(self), c920_rtsp_server_get_service(self));

	g_print("Started RTSP server on %s:%s\n", c920_rtsp_server_get_address(self), c920_rtsp_server_get_service(self));
	g_timeout_add_seconds(2, (GSourceFunc)c920_rtsp_server_timeout, self);

	return r;
}

/* Forwarded functions */

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


