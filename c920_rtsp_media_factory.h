#ifndef C920_RTSP_MEDIA_FACTORY_H
#define C920_RTSP_MEDIA_FACTORY_H

#include "c920_global.h"
#include "c920_live_src.h"

#define C920_TYPE_RTSP_MEDIA_FACTORY \
	(c920_rtsp_media_factory_get_type())

#define C920_RTSP_MEDIA_FACTORY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactory))

#define C920_IS_RTSP_MEDIA_FACTORY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_RTSP_MEDIA_FACTORY))

#define C920_RTSP_MEDIA_FACTORY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactoryClass))

#define C920_IS_RTSP_MEDIA_FACTORY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_RTSP_MEDIA_FACTORY))

#define C920_RTSP_MEDIA_FACTORY_GET_CLASS(obj) \
	(G_TYPE_GET_INSTANCE_CLASS((obj), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactoryClass))

typedef struct _C920RtspMediaFactory 		C920RtspMediaFactory;
typedef struct _C920RtspMediaFactoryClass	C920RtspMediaFactoryClass;
typedef struct _C920RtspMediaFactoryPrivate	C920RtspMediaFactoryPrivate;

struct _C920RtspMediaFactory
{
	GstRTSPMediaFactory parent_instance;
	C920RtspMediaFactoryPrivate *priv;
};

struct _C920RtspMediaFactoryClass
{
	GstRTSPMediaFactoryClass parent_class;
};

GType c920_rtsp_media_factory_get_type();

#endif
