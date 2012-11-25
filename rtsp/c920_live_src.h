#ifndef C920_LIVE_SRC_H
#define C920_LIVE_SRC_H

#include "c920_global.h"

#define C920_TYPE_LIVE_SRC \
	(c920_live_src_get_type())

#define C920_LIVE_SRC(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_LIVE_SRC, C920LiveSrc))

#define C920_IS_LIVE_SRC(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_LIVE_SRC))

#define C920_LIVE_SRC_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_LIVE_SRC, C920LiveSrcClass))

#define C920_IS_LIVE_SRC_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_LIVE_SRC))

#define C920_LIVE_SRC_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), C920_TYPE_LIVE_SRC, C920LiveSrcClass))

typedef struct _C920LiveSrc		C920LiveSrc;
typedef struct _C920LiveSrcClass	C920LiveSrcClass;
typedef struct _C920LiveSrcPrivate	C920LiveSrcPrivate;

struct _C920LiveSrc
{
	GstAppSrc parent_instance;
	C920LiveSrcPrivate *priv;
};

struct _C920LiveSrcClass
{
	GstAppSrcClass parent_class;
};

GType c920_live_src_get_type();


#endif
