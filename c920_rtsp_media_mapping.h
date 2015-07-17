#ifndef C920_RTSP_MEDIA_MAPPING_H
#define C920_RTSP_MEDIA_MAPPING_H

#include "c920_global.h"
#include "c920_rtsp_media_factory.h"

#define C920_TYPE_RTSP_MEDIA_MAPPING \
	(c920_rtsp_media_mapping_get_type())

#define C920_RTSP_MEDIA_MAPPING(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_RTSP_MEDIA_MAPPING, C920RtspMediaMapping))

#define C920_IS_RTSP_MEDIA_MAPPING(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_RTSP_MEDIA_MAPPING))

#define C920_RTSP_MEDIA_MAPPING_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_RTSP_MEDIA_MAPPING, C920RtspMediaMappingClass))

#define C920_IS_RTSP_MEDIA_MAPPING_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_RTSP_MEDIA_MAPPING))

#define C920_RTSP_MEDIA_MAPPING_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), C920_TYPE_RTSP_MEDIA_MAPPING, C920_RtspMediaMappingClass))


typedef struct _C920RtspMediaMapping		C920RtspMediaMapping;
typedef struct _C920RtspMediaMappingClass	C920RtspMediaMappingClass;
typedef struct _C920RtspMediaMappingPrivate	C920RtspMediaMappingPrivate;

struct _C920RtspMediaMapping
{
	GstRTSPMediaMapping parent_instance;
	C920RtspMediaMappingPrivate *priv;
};

struct _C920RtspMediaMappingClass
{
	GstRTSPMediaMappingClass parent_class;
};

GType c920_rtsp_media_mapping_get_type();

#endif
