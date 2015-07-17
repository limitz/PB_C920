#include "c920_device_manager.h"

G_DEFINE_TYPE(C920DeviceManager, c920_device_manager, G_TYPE_OBJECT);

#define C920_DEVICE_MANAGER_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_DEVICE_MANAGER, C920DeviceManagerPrivate))

struct _C920DeviceManagerPrivate
{
        GHashTable *devices;
};


/* Forward Declarations */
static GObject*	c920_device_manager_construct(GType, guint, GObjectConstructParam*);
static void     c920_device_manager_class_init(C920DeviceManagerClass*);
static void     c920_device_manager_init(C920DeviceManager*);
static void     c920_device_manager_dispose(GObject*);
static void     c920_device_manager_finalize(GObject*);


/* Compare 2 strings, needed for the keys in the devices hashtable */
static gboolean	string_equals(gconstpointer a, gconstpointer b)
{
	return (gboolean)(!strcmp((const gchar*)a, (const gchar*)b));
}


/* C920 Device Manager Singleton Constructor
 * -----------------------------------------
 * Multithread-safe constructor which will only allow one instance to be allocated at one time.
 * Once the last instance is unreffed, the instance will be destroyed. The next call will create
 * a new singleton instance
 */
static GObject* c920_device_manager_construct(GType type, guint n_params, GObjectConstructParam *params)
{
	static GMutex mutex = { NULL };
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


/* C920 Device Manager Class Initializer
 * -------------------------------------
 * Setup a custom constructor for the singleton functionality
 */
static void c920_device_manager_class_init(C920DeviceManagerClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);
	object_class->constructor  = c920_device_manager_construct;
	object_class->dispose      = c920_device_manager_dispose;
	object_class->finalize     = c920_device_manager_finalize;

	g_type_class_add_private(cls, sizeof(C920DeviceManagerPrivate));
}


/* C920 Device Manager Instance Initializer
 * ----------------------------------------
 * Set up the devices hash table
 */
static void c920_device_manager_init(C920DeviceManager *self)
{
	self->priv = C920_DEVICE_MANAGER_GET_PRIVATE(self);
	self->priv->devices = g_hash_table_new_full(g_str_hash, string_equals, g_free, g_object_unref); 
}


/* C920 Device Manager Instance Dispose
 * ------------------------------------
 * Destroy the devices hash table. All elements will be unreffed
 */
static void c920_device_manager_dispose(GObject *obj)
{
	C920DeviceManager *self = C920_DEVICE_MANAGER(obj);
	g_hash_table_destroy(self->priv->devices);
	G_OBJECT_CLASS(c920_device_manager_parent_class)->dispose(obj);
}


/* C920 Device Manager Instance Finalize
 * -------------------------------------
 * Nothing to do for now, kept for possible updates
 */
static void c920_device_manager_finalize(GObject *obj)
{
	C920DeviceManager *self __unused = C920_DEVICE_MANAGER(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_device_manager_parent_class)->finalize(obj);
}


/* C920 Device Manager - Add a device
 * ----------------------------------
 * Add a device to the hashtable. After this, the device will be
 * available for other classes by its 'name'.
 * The device will be reffed before insertion so it's safe to
 * unref the device after adding it to the manager.
 */
void c920_device_manager_add_device(C920DeviceManager *self, const gchar *name, C920VideoDevice *device)
{
	g_return_if_fail(C920_IS_DEVICE_MANAGER(self));
	g_hash_table_insert(self->priv->devices, g_strdup(name), g_object_ref(device));
}


/* C920 Device Manager - Get a device
 * ----------------------------------
 * Get a device that was previously added. Returns NULL if name is
 * not present in the hashtable. The result needs to be unreffed
 * after the caller is done with it.
 */
C920VideoDevice* c920_device_manager_get_device(C920DeviceManager *self, const gchar *name)
{
	if (!C920_IS_DEVICE_MANAGER(self)) return NULL;

	gpointer *result = g_hash_table_lookup(self->priv->devices, name);
	if (result) 
	{
		g_debug("DEVICE MANAGER: Found device: %s\n", name);
		g_object_ref(result);
		return (C920VideoDevice*)result;
	}
	else g_warning("DEVICE MANAGER: Unable to find device: %s\n", name);
	return NULL;
}

/* C920 Device Manager - Remove a device
 * -------------------------------------
 * Remove a device from the hashtable. Also unrefs the device.
 */
void c920_device_manager_remove_device(C920DeviceManager *self, const gchar *name)
{
	g_hash_table_remove(self->priv->devices, name);
}

