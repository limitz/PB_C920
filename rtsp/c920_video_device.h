#ifndef C920_VIDEO_DEVICE_H
#define C920_VIDEO_DEVICE_H

#include "c920_global.h"

#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>


#define C920_TYPE_VIDEO_DEVICE \
	(c920_video_device_get_type())

#define C920_VIDEO_DEVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_VIDEO_DEVICE, C920VideoDevice))

#define C920_IS_VIDEO_DEVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_VIDEO_DEVICE))

#define C920_VIDEO_DEVICE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_VIDEO_DEVICE, C920VideoDeviceClass))

#define C920_IS_VIDEO_DEVICE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_VIDEO_DEVICE))

#define C920_VIDEO_DEVICE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), C920_TYPE_VIDEO_DEVICE, C920VideoDeviceClass))

typedef void (*C920VideoBufferFunc)(void* data, size_t length, gpointer userdata);

typedef struct _C920VideoDevice		C920VideoDevice;
typedef struct _C920VideoDeviceClass	C920VideoDeviceClass;
typedef struct _C920VideoDevicePrivate	C920VideoDevicePrivate;

struct _C920VideoDevice
{
	GObject parent_instance;
	C920VideoDevicePrivate *priv;
};

struct _C920VideoDeviceClass
{
	GObjectClass parent_class;
};

GType 
c920_video_device_get_type();

const gchar* 	
c920_video_device_get_device_name(C920VideoDevice*);

void 		
c920_video_device_set_width(C920VideoDevice*, guint);

guint		
c920_video_device_get_width(C920VideoDevice*);

void		
c920_video_device_set_height(C920VideoDevice*, guint);

guint		
c920_video_device_get_height(C920VideoDevice*);

void		
c920_video_device_set_fps(C920VideoDevice*, guint);

guint		
c920_video_device_get_fps(C920VideoDevice*);

void
c920_video_device_set_dump_file_name(C920VideoDevice*, const gchar*);

const gchar*
c920_video_device_get_dump_file_name(C920VideoDevice*);

gboolean
c920_video_device_start(C920VideoDevice*);

gboolean
c920_video_device_stop(C920VideoDevice*);

void 
c920_video_device_set_callback(C920VideoDevice*, C920VideoBufferFunc, gpointer);

#endif
