#include "c920_rtsp_media_mapping.h"

G_DEFINE_TYPE(C920RtspMediaMapping, c920_rtsp_media_mapping, GST_TYPE_RTSP_MEDIA_MAPPING);

#define C920_RTSP_MEDIA_MAPPING_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_MEDIA_MAPPING, C920RtspMediaMappingPrivate))

struct _C920RtspMediaMappingPrivate
{
	int dummy;
};


/* Forward Declarations */
static void c920_rtsp_media_mapping_class_init(C920RtspMediaMappingClass*);
static void c920_rtsp_media_mapping_init(C920RtspMediaMapping*);
static void c920_rtsp_media_mapping_dispose(GObject*);
static void c920_rtsp_media_mapping_finalize(GObject*);

/* GstRTSPMediaFactory overloads */
static GstRTSPMediaFactory* c920_rtsp_media_mapping_real_find_media(GstRTSPMediaMapping*, const GstRTSPUrl*);


/* C920 RTSP Media Mapping Class Initializer
 * -----------------------------------------
 * Overload the find-media function for the GstRTSPMediaMapping
 */
static void c920_rtsp_media_mapping_class_init(C920RtspMediaMappingClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);
	GstRTSPMediaMappingClass *base_class = GST_RTSP_MEDIA_MAPPING_CLASS(cls);

	object_class->dispose  = c920_rtsp_media_mapping_dispose;
	object_class->finalize = c920_rtsp_media_mapping_finalize;

	base_class->find_media = c920_rtsp_media_mapping_real_find_media;

	g_type_class_add_private(cls, sizeof(C920RtspMediaMapping));
}


/* C920 RTSP Media Mapping Instance Initializer
 * --------------------------------------------
 * Nothing special here
 */
static void c920_rtsp_media_mapping_init(C920RtspMediaMapping *self)
{
	self->priv = C920_RTSP_MEDIA_MAPPING_GET_PRIVATE(self);
}


/* C920 RTSP Media Mapping Dispose
 * -------------------------------
 * Nothing to do for now
 */
static void c920_rtsp_media_mapping_dispose(GObject *obj)
{
	C920RtspMediaMapping *self __unused =  C920_RTSP_MEDIA_MAPPING(obj);
	/* Nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_mapping_parent_class)->dispose(obj);
}


/* C920 RTSP Media Mapping Finalize
 * --------------------------------
 * Nothing to do for now
 */
static void c920_rtsp_media_mapping_finalize(GObject *obj)
{
	C920RtspMediaMapping *self __unused = C920_RTSP_MEDIA_MAPPING(obj);
	/* Nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_mapping_parent_class)->finalize(obj);
}


/* C920 RTSP Media Mapping - Find media
 * ------------------------------------
 * Overload the protected find-media function.
 * This will create a new RTSP Media Factory, based on the abspath.
 * The abspath should be the device name without the /dev. So /dev/video0 
 * should be /video0
 */
static GstRTSPMediaFactory* c920_rtsp_media_mapping_real_find_media(GstRTSPMediaMapping *base, const GstRTSPUrl* url)
{
	GstRTSPMediaFactory *result = GST_RTSP_MEDIA_MAPPING_CLASS(c920_rtsp_media_mapping_parent_class)->find_media(base, url);
	if (!result)
	{
		result = GST_RTSP_MEDIA_FACTORY(g_object_new(C920_TYPE_RTSP_MEDIA_FACTORY,NULL));
		gst_rtsp_media_mapping_add_factory(base, url->abspath, result);
		g_object_ref(result);
	}	
	return result;
}
