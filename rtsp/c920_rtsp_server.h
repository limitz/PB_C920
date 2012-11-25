#ifndef C920_RTSP_SERVER_H
#define C920_RTSP_SERVER_H

#include "c920_global.h"
#include "c920_rtsp_media_mapping.h"

#define C920_TYPE_RTSP_SERVER \
	(c920_rtsp_server_get_type())

#define C920_RTSP_SERVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_RTSP_SERVER, C920RtspServer))

#define C920_IS_RTSP_SERVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_RTSP_SERVER))

#define C920_RTSP_SERVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_RTSP_SERVER, C920RtspServerClass))

#define C920_IS_RTSP_SERVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_RTSP_SERVER))
	
#define C920_RTSP_SERVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), C920_TYPE_RTSP_SERVER, C920RtspServerClass))


typedef struct _C920RtspServer		C920RtspServer;
typedef struct _C920RtspServerClass	C920RtspServerClass;
typedef struct _C920RtspServerPrivate	C920RtspServerPrivate;

struct _C920RtspServer
{
	GstRTSPServer parent_instance;
	C920RtspServerPrivate *priv;
};

struct _C920RtspServerClass
{
	GstRTSPServerClass parent_class;
};

GType c920_rtsp_server_get_type();

#endif
