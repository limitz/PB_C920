#ifndef C920_DEVICE_MANAGER_H
#define C920_DEVICE_MANAGER_H

#include "c920_global.h"
#include "c920_video_device.h"


#define C920_TYPE_DEVICE_MANAGER \
	(c920_device_manager_get_type())

#define C920_DEVICE_MANAGER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), C920_TYPE_DEVICE_MANAGER, C920DeviceManager))

#define C920_IS_DEVICE_MANAGER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), C920_TYPE_DEVICE_MANAGER))

#define C920_DEVICE_MANAGER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST((cls), C920_TYPE_DEVICE_MANAGER, C920DeviceManagerClass))

#define C920_IS_DEVICE_MANAGER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE((cls), C920_TYPE_DEVICE_MANAGER))

#define C920_DEVICE_MANAGER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), C920_TYPE_DEVICE_MANAGER, C920DeviceManagerClass))

typedef struct _C920DeviceManager 		C920DeviceManager;
typedef struct _C920DeviceManagerClass		C920DeviceManagerClass;
typedef struct _C920DeviceManagerPrivate	C920DeviceManagerPrivate;

struct _C920DeviceManager
{
	GObject parent_instance;
	C920DeviceManagerPrivate *priv;
};

struct _C920DeviceManagerClass
{
	GObjectClass parent_class;
};

GType 
c920_device_manager_get_type();

void 
c920_device_manager_add_device(C920DeviceManager*, const gchar*, C920VideoDevice*);

C920VideoDevice*
c920_device_manager_get_device(C920DeviceManager*, const gchar*);

void
c920_device_manager_remove_device(C920DeviceManager*, const gchar*);

#endif
