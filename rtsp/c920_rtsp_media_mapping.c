#include "c920_rtsp_media_mapping.h"

#define C920_RTSP_MEDIA_MAPPING_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_MEDIA_MAPPING, C920RtspMediaMappingPrivate))


G_DEFINE_TYPE(C920RtspMediaMapping, c920_rtsp_media_mapping, GST_TYPE_RTSP_MEDIA_MAPPING);


struct _C920RtspMediaMappingPrivate
{
	int dummy;
};

static void c920_rtsp_media_mapping_class_init(C920RtspMediaMappingClass*);
static void c920_rtsp_media_mapping_init(C920RtspMediaMapping*);
static void c920_rtsp_media_mapping_dispose(GObject*);
static void c920_rtsp_media_mapping_finalize(GObject*);

static GstRTSPMediaFactory* c920_rtsp_media_mapping_real_find_media(GstRTSPMediaMapping*, const GstRTSPUrl*);

static void c920_rtsp_media_mapping_class_init(C920RtspMediaMappingClass *cls)
{
	G_OBJECT_CLASS(cls)->dispose = c920_rtsp_media_mapping_dispose;
	G_OBJECT_CLASS(cls)->finalize = c920_rtsp_media_mapping_finalize;

	GST_RTSP_MEDIA_MAPPING_CLASS(cls)->find_media = c920_rtsp_media_mapping_real_find_media;

	g_type_class_add_private(cls, sizeof(C920RtspMediaMapping));
}

static void c920_rtsp_media_mapping_init(C920RtspMediaMapping *self)
{
	self->priv = C920_RTSP_MEDIA_MAPPING_GET_PRIVATE(self);
}

static void c920_rtsp_media_mapping_dispose(GObject *obj)
{
	C920RtspMediaMapping *self __unused =  C920_RTSP_MEDIA_MAPPING(obj);
	/* todo */
	G_OBJECT_CLASS(c920_rtsp_media_mapping_parent_class)->dispose(obj);
}

static void c920_rtsp_media_mapping_finalize(GObject *obj)
{
	C920RtspMediaMapping *self __unused = C920_RTSP_MEDIA_MAPPING(obj);
	/* todo */
	G_OBJECT_CLASS(c920_rtsp_media_mapping_parent_class)->finalize(obj);
}

static GstRTSPMediaFactory* c920_rtsp_media_mapping_real_find_media(GstRTSPMediaMapping *base, const GstRTSPUrl* url)
{
	GstRTSPMediaFactory* result = GST_RTSP_MEDIA_MAPPING_CLASS(c920_rtsp_media_mapping_parent_class)->find_media(base, url);
	if (!result)
	{
		result = GST_RTSP_MEDIA_FACTORY(g_object_new(C920_TYPE_RTSP_MEDIA_FACTORY,NULL));
		gst_rtsp_media_mapping_add_factory(base, url->abspath, result);
		g_object_ref(result);
	}
	return result;
}
