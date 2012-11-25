#include "c920_rtsp_media_factory.h"

#define C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_RTSP_MEDIA_FACTORY, C920RtspMediaFactoryPrivate))


G_DEFINE_TYPE(C920RtspMediaFactory, c920_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);


struct _C920RtspMediaFactoryPrivate
{
	int dummy;
};

static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass*);
static void c920_rtsp_media_factory_init(C920RtspMediaFactory*);
static void c920_rtsp_media_factory_dispose(GObject*);
static void c920_rtsp_media_factory_finalize(GObject*);

static GstRTSPMedia* 	c920_rtsp_media_factory_real_construct(GstRTSPMediaFactory*, const GstRTSPUrl*);
static GstElement*   	c920_rtsp_media_factory_real_create_pipeline(GstRTSPMediaFactory*, GstRTSPMedia*);
static void		c920_rtsp_media_factory_real_configure(GstRTSPMediaFactory*, GstRTSPMedia*);
static GstElement*	c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory*, const GstRTSPUrl*);
static gchar*		c920_rtsp_media_factory_real_gen_key(GstRTSPMediaFactory*, const GstRTSPUrl*);


static void c920_rtsp_media_factory_class_init(C920RtspMediaFactoryClass *cls)
{
	GstRTSPMediaFactoryClass* base = GST_RTSP_MEDIA_FACTORY_CLASS(cls);

	base->construct 	= c920_rtsp_media_factory_real_construct;
	base->create_pipeline 	= c920_rtsp_media_factory_real_create_pipeline;
	base->configure		= c920_rtsp_media_factory_real_configure;
	base->get_element	= c920_rtsp_media_factory_real_get_element;
	base->gen_key		= c920_rtsp_media_factory_real_gen_key;

	G_OBJECT_CLASS(cls)->dispose = c920_rtsp_media_factory_dispose;
	G_OBJECT_CLASS(cls)->finalize = c920_rtsp_media_factory_finalize;

	g_type_class_add_private(cls, sizeof(C920RtspMediaFactoryPrivate));
}

static void c920_rtsp_media_factory_init(C920RtspMediaFactory *self)
{
	self->priv = C920_RTSP_MEDIA_FACTORY_GET_PRIVATE(self);
	gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(self), TRUE);
}

static void c920_rtsp_media_factory_dispose(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->dispose(obj);
}

static void c920_rtsp_media_factory_finalize(GObject *obj)
{
	C920RtspMediaFactory *self __unused = C920_RTSP_MEDIA_FACTORY(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_rtsp_media_factory_parent_class)->finalize(obj);
}

static GstRTSPMedia* c920_rtsp_media_factory_real_construct(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->construct(base, url);
}

static GstElement* c920_rtsp_media_factory_real_create_pipeline(GstRTSPMediaFactory *base, GstRTSPMedia *media)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->create_pipeline(base, media);
}

static void c920_rtsp_media_factory_real_configure(GstRTSPMediaFactory *base, GstRTSPMedia *media)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->configure(base, media);
}

static GstElement* c920_rtsp_media_factory_real_get_element(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// todo: add own logic
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->get_element(base, url);
}

static gchar* c920_rtsp_media_factory_real_gen_key(GstRTSPMediaFactory *base, const GstRTSPUrl *url)
{
	// todo: add own logid
	return GST_RTSP_MEDIA_FACTORY_CLASS(c920_rtsp_media_factory_parent_class)->gen_key(base, url);
}
