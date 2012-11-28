#include "c920_device_manager.h"

#define C920_DEVICE_MANAGER_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_DEVICE_MANAGER, C920DeviceManagerPrivate))


G_DEFINE_TYPE(C920DeviceManager, c920_device_manager, G_TYPE_OBJECT);

struct _C920DeviceManagerPrivate
{
        GHashTable *devices;
};

static GObject*	c920_device_manager_construct(GType, guint, GObjectConstructParam*);
static void     c920_device_manager_class_init(C920DeviceManagerClass*);
static void     c920_device_manager_init(C920DeviceManager*);
static void     c920_device_manager_dispose(GObject*);
static void     c920_device_manager_finalize(GObject*);

static gboolean	c920_device_manager_equal_func(gconstpointer, gconstpointer);

static GMutex mutex = { NULL };

static GObject* c920_device_manager_construct(GType type, guint n_params, GObjectConstructParam *params)
{
	static GObject *self = NULL;
	g_mutex_lock(&mutex);
	if (self == NULL)
	{
		self = G_OBJECT_CLASS(c920_device_manager_parent_class)->constructor(type, n_params, params);
		g_object_add_weak_pointer(self, (gpointer) self);
		g_mutex_unlock(&mutex);
		return self;
	}
	g_mutex_unlock(&mutex);
	return g_object_ref(self);
}

static void c920_device_manager_class_init(C920DeviceManagerClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);

	object_class->constructor = c920_device_manager_construct;
	object_class->dispose = c920_device_manager_dispose;
	object_class->finalize = c920_device_manager_finalize;
}

static void c920_device_manager_init(C920DeviceManager *self)
{
	self->priv = C920_DEVICE_MANAGER_GET_PRIVATE(self);

	self->priv->devices = g_hash_table_new_full(g_str_hash, c920_device_manager_equal_func, g_free, g_object_unref); 
}

static void c920_device_manager_dispose(GObject *obj)
{
	C920DeviceManager *self = C920_DEVICE_MANAGER(obj);
	g_hash_table_destroy(self->priv->devices);
	G_OBJECT_CLASS(c920_device_manager_parent_class)->dispose(obj);
}

static void c920_device_manager_finalize(GObject *obj)
{
	C920DeviceManager *self __unused = C920_DEVICE_MANAGER(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_device_manager_parent_class)->finalize(obj);
}

void c920_device_manager_add_device(C920DeviceManager *self, const gchar *name, C920VideoDevice *device)
{
	g_return_if_fail(C920_IS_DEVICE_MANAGER(self));
	g_hash_table_insert(self->priv->devices, g_strdup(name), g_object_ref(device));
}

C920VideoDevice* c920_device_manager_get_device(C920DeviceManager *self, const gchar *name)
{
	if (!C920_IS_DEVICE_MANAGER(self)) return NULL;
	return g_hash_table_lookup(self->priv->devices, name);
}

void c920_device_manager_remove_device(C920DeviceManager *self, const gchar *name)
{
	g_hash_table_remove(self->priv->devices, name);
}

static gboolean c920_device_manager_equal_func(gconstpointer a, gconstpointer b)
{
	return (gboolean)(!strcmp((const gchar*)a, (const gchar*) b));
}
