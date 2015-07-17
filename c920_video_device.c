#include "c920_video_device.h"

G_DEFINE_TYPE(C920VideoDevice, c920_video_device, G_TYPE_OBJECT);

#define C920_VIDEO_DEVICE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_VIDEO_DEVICE, C920VideoDevicePrivate))
enum 
{
	PROP_0,
	PROP_DEVICE_NAME,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_FPS,
	PROP_DUMP_FILE_NAME,
	N_PROPERTIES
};

static GMutex mutex = { NULL };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL };

struct c920_buffer
{
        void *data;
        size_t length;
};

struct c920_callback
{
	C920VideoBufferFunc cb;
	gpointer userdata;
};

struct _C920VideoDevicePrivate
{
	// properties
	gchar *device_name, *dump_file_name;
	guint width, height, fps;
	GHashTable *hashtable;	
	
	// device io
	int fd;
	struct c920_buffer *buffers;
	size_t num_buffers;
	FILE *dump_file;

	int start_count;
	GThread *thread;
};

/* Forward Declarations */
static void c920_video_device_class_init(C920VideoDeviceClass*);
static void c920_video_device_init(C920VideoDevice*);
static void c920_video_device_get_property(GObject*, guint, GValue*, GParamSpec*);
static void c920_video_device_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void c920_video_device_dispose(GObject*);
static void c920_video_device_finalize(GObject*);

/* Constructor only property setters */
static void c920_video_device_set_device_name(C920VideoDevice*, const gchar*);
static void c920_video_device_set_width(C920VideoDevice*, guint);
static void c920_video_device_set_height(C920VideoDevice*, guint);
static void c920_video_device_set_fps(C920VideoDevice*, guint);

/* Main loop process function */
static gpointer c920_video_device_process(gpointer userdata);

/* Equals function for hashtable */
static gboolean g_pointer_equals(gconstpointer a, gconstpointer b)
{
	return a == b;
}

/* IOCTL function for device IO */
static int ioctl_ex(int fh, int request, void* arg)
{
	int r;
	do { r = ioctl(fh, request, arg); } while (r == -1 && EINTR == errno);
	return r;
}


/* C920 Video Device Class Initializer
 * -----------------------------------
 * Set property declarations
 */
static void c920_video_device_class_init(C920VideoDeviceClass *cls)
{
	GObjectClass *base = G_OBJECT_CLASS(cls);

	base->set_property = c920_video_device_set_property;
	base->get_property = c920_video_device_get_property;
	base->dispose = c920_video_device_dispose;
	base->finalize = c920_video_device_finalize;

	obj_properties[PROP_DEVICE_NAME] = 
		g_param_spec_string(
			"device-name", 
			"Video device name", 
			"Set the video device name", 
			"/dev/video0",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE); 

	obj_properties[PROP_WIDTH] = 
		g_param_spec_uint(
			"width",
			"Frame width",
			"Set the frame width",
			0, 1920, 1280,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	obj_properties[PROP_HEIGHT] =
		g_param_spec_uint(
			"height",
			"Frame height",
			"Set the frame height",
			0, 1080, 720,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	obj_properties[PROP_FPS] = 
		g_param_spec_uint(
			"fps",
			"Frames per second",
			"Set the frames per second",
			1, 30, 25,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	obj_properties[PROP_DUMP_FILE_NAME] = 
		g_param_spec_string(
			"dump-file-name",
			"H.264 dump file",
			"Set the file where the device should dump a copy of its h.264 data to",
			"/dev/null",
			G_PARAM_READWRITE);

	g_object_class_install_properties(base, N_PROPERTIES, obj_properties);
	g_type_class_add_private(cls, sizeof(C920VideoDevicePrivate));
}


/* C920 Video Device Instance Initializer
 * --------------------------------------
 * Initialize the hashtable
 */
static void c920_video_device_init(C920VideoDevice *self)
{
	self->priv = C920_VIDEO_DEVICE_GET_PRIVATE(self);
	self->priv->hashtable = g_hash_table_new_full(g_direct_hash, g_pointer_equals, NULL, g_free);
}


/* C920 Video Device Property Getter
 * ---------------------------------
 * Get the device-name, width, height, fps and dump-file-name
 */
static void c920_video_device_get_property(GObject* obj, guint pidx, GValue* value, GParamSpec* spec)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);

	switch (pidx)
	{
		case PROP_DEVICE_NAME:
			g_value_set_string(value, self->priv->device_name);
			break;

		case PROP_WIDTH:
			g_value_set_uint(value, c920_video_device_get_width(self));
			break;

		case PROP_HEIGHT:
			g_value_set_uint(value, c920_video_device_get_height(self));
			break;

		case PROP_FPS:
			g_value_set_uint(value, c920_video_device_get_fps(self));
			break;

		case PROP_DUMP_FILE_NAME:
			g_value_set_string(value, c920_video_device_get_dump_file_name(self));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, pidx, spec);
			break;
	}
}


/* C920 Video Device Property Setter
 * ---------------------------------
 * Set from constructor only: device-name, width, height, fps
 * Set from anywhere: file-dump-name.
 * The file dump name can be changed during operation, so that the dump can be split
 */
static void c920_video_device_set_property(GObject *obj, guint pidx, const GValue *value, GParamSpec *spec)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);

	switch (pidx)
	{
		case PROP_DEVICE_NAME:
			c920_video_device_set_device_name(self, g_value_get_string(value));
			break;

		case PROP_WIDTH:
			c920_video_device_set_width(self, g_value_get_uint(value));
			break;

		case PROP_HEIGHT:
			c920_video_device_set_height(self, g_value_get_uint(value));
			break;

		case PROP_FPS:
			c920_video_device_set_fps(self, g_value_get_uint(value));
			break;

		case PROP_DUMP_FILE_NAME:
			c920_video_device_set_dump_file_name(self, g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, pidx, spec);
			break;
	}
}


/* C920 Video Device Dispose
 * -------------------------
 * Destroy the hashtable
 */
static void c920_video_device_dispose(GObject *obj)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);

	if (self->priv->dump_file)
        {
                g_mutex_lock(&mutex);
                fclose(self->priv->dump_file);
                self->priv->dump_file = NULL;
                g_mutex_unlock(&mutex);
        }

	g_hash_table_destroy(self->priv->hashtable);
	G_OBJECT_CLASS(c920_video_device_parent_class)->dispose(obj);
}


/* C920 Video Device Finalize
 * --------------------------
 * Free the device name and the dump file name
 * Also close the dump file*/
static void c920_video_device_finalize(GObject *obj)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);
	g_free(self->priv->device_name);
	g_free(self->priv->dump_file_name);
	
	G_OBJECT_CLASS(c920_video_device_parent_class)->finalize(obj);
}


/* Property Setters / Getters */
static void c920_video_device_set_width(C920VideoDevice *self, guint width)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(width > 0 && width <= 1920);

	self->priv->width = width;
}

guint c920_video_device_get_width(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return -1;
	return self->priv->width;
}

static void c920_video_device_set_height(C920VideoDevice *self, guint height)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(height > 0 && height <= 1080);

	self->priv->height = height;
}

guint c920_video_device_get_height(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return -1;
	return self->priv->height;
}

static void c920_video_device_set_fps(C920VideoDevice *self, guint fps)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(fps > 0 && fps <= 30);

	self->priv->fps = fps;
}

guint c920_video_device_get_fps(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return -1;
	return self->priv->fps;
}

void c920_video_device_set_dump_file_name(C920VideoDevice *self, const gchar *name)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));

	g_free(self->priv->dump_file_name);
	self->priv->dump_file_name = g_strdup(name);

	g_mutex_lock(&mutex);
	if (self->priv->dump_file) fclose(self->priv->dump_file);
	if (name) self->priv->dump_file = fopen(name, "wb");
	else self->priv->dump_file = NULL;
	g_mutex_unlock (&mutex);
}

const gchar* c920_video_device_get_dump_file_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->dump_file_name;
}


/* C920 Video Device - Add Callback
 * --------------------------------
 * Add a C920VideoBufferFunc callback to the frame capture process.
 * userdata will be used as the key in the hashtable, so make it unique
 */
void c920_video_device_add_callback(C920VideoDevice *self, C920VideoBufferFunc callback, gpointer userdata)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	
	g_mutex_lock(&mutex);
	struct c920_callback *cb = g_malloc(sizeof(struct c920_callback));
	cb->cb = callback;
	cb->userdata = userdata;
	g_hash_table_insert(self->priv->hashtable, userdata, cb);
	g_mutex_unlock(&mutex);
}


/* C920 Video Device - Remove Callback
 * -----------------------------------
 * Remove a callback by the userdata passed in the add function
 */
void c920_video_device_remove_callback_by_data(C920VideoDevice *self, gpointer userdata)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_mutex_lock(&mutex);
	g_hash_table_remove(self->priv->hashtable, userdata);
	g_mutex_unlock(&mutex);
}

/* Set the device name, constructor only */
static void c920_video_device_set_device_name(C920VideoDevice *self, const gchar* device_name)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(device_name != NULL);

	g_free(self->priv->device_name);
	self->priv->device_name = g_strdup(device_name);
}

/* Get the device name */
const gchar* c920_video_device_get_device_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->device_name;
}


/* DEVICE IO */

/* C920 Video Device - Close
 * -------------------------
 * Close the device and unmap memory
 */
static gboolean c920_video_device_close(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
	
	size_t i;
	
	if (self->priv->fd && close(self->priv->fd) == -1) g_warning("Unable to close device: %s", self->priv->device_name);
	self->priv->fd = 0;
	
	for (i = 0; i < self->priv->num_buffers; i++)
	{
		if (munmap(self->priv->buffers[i].data, self->priv->buffers[i].length) == -1)
		g_warning("Unable to unmap buffer %d on device %s", i, self->priv->device_name);
        }
	
	free(self->priv->buffers);
	self->priv->buffers = NULL;
	self->priv->num_buffers = 0;

	return TRUE;
}

/* C920 Video Device - Open
 * ------------------------
 * Open a handle to the device
 */
static gboolean c920_video_device_open(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
	if (self->priv->fd) c920_video_device_close(self);

	int fd;
	struct stat st;	
	struct v4l2_capability cap;
	const char *device_name = self->priv->device_name;
	
	g_debug("Opening video device %s", device_name);

	if (stat(device_name, &st) == -1) 	
		WARNING_RETURN(FALSE, "Unable to identify device: %s", device_name);

	if (!S_ISCHR(st.st_mode)) 
		WARNING_RETURN(FALSE, "Not a device: %s", device_name);

	if ((fd = open(device_name, O_RDWR | O_NONBLOCK, 0)) == -1)
		WARNING_RETURN(FALSE, "Cannot open device: %s", device_name);
		
        if (ioctl_ex(fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		close(fd);
		WARNING_RETURN(FALSE, "Not a valid V4L2 device: %s", device_name);
	}
	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		close(fd);
		WARNING_RETURN(FALSE, "Not a capture device: %s", device_name);
	}
	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		close(fd);
		WARNING_RETURN(FALSE, "Not a streaming device: %s", device_name);
	}

	self->priv->fd = fd;
	return TRUE;
}	

/* C920 Video Device - Setup
 * -------------------------
 * Set up the stream parameters like width, height and fps
 */
static gboolean c920_video_device_setup_stream(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
	
	int fd = self->priv->fd;
	const char* device_name = self->priv->device_name;
	struct v4l2_format fmt;
	struct v4l2_streamparm params;
	enum   v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (!fd) WARNING_RETURN(FALSE, "Device has not been opened: %s", device_name);
	
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = type;
	fmt.fmt.pix.width = self->priv->width;
	fmt.fmt.pix.height = self->priv->height;
	fmt.fmt.pix.pixelformat = v4l2_fourcc('H', '2', '6', '4');
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	if (ioctl_ex(fd, VIDIOC_S_FMT, &fmt) == -1)
		WARNING_RETURN(FALSE, "Unable to set stream format for device: %s", device_name);

	memset(&params, 0, sizeof(params));
	params.type = type;

	if (ioctl_ex(fd, VIDIOC_G_PARM, &params) == -1)
		WARNING_RETURN(FALSE, "Unable to get stream parameters for device: %s", device_name);

	params.parm.capture.timeperframe.numerator = 1;
	params.parm.capture.timeperframe.denominator = self->priv->fps;

	if (ioctl_ex(fd, VIDIOC_S_PARM, &params) == -1)
		WARNING_RETURN(FALSE, "Unable to set stream parameters for device: %s", device_name);

	return TRUE;		
}

/* C920 Video Device - Memory
 * --------------------------
 * Set up memory mapping for the device
 */
static gboolean c920_video_device_memory_map(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;

	int i;
	struct v4l2_requestbuffers request;
	int fd = self->priv->fd;
	void* data;
	const char* device_name = self->priv->device_name;
	enum   v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (!fd) WARNING_RETURN(FALSE, "Device has not been opened: %s", device_name);

	memset(&request, 0, sizeof(request));
	request.count = 4;
	request.type = type;
	request.memory = V4L2_MEMORY_MMAP;

	if (ioctl_ex(fd, VIDIOC_REQBUFS, &request) == -1)
		WARNING_RETURN(FALSE, "Memory mapping not supported on device: %s", device_name);

	if (request.count < 2)
		WARNING_RETURN(FALSE, "Insufficient memory on device: %s", device_name);

	if (!(self->priv->buffers = (struct c920_buffer*) calloc(request.count, sizeof(struct c920_buffer))))
		WARNING_RETURN(FALSE, "Out of memory trying to allocate %d bytes", request.count * sizeof(struct c920_buffer));

	for (i = 0; i < request.count; i++)
	{
		struct v4l2_buffer buf = {0};
		buf.type = type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl_ex(fd, VIDIOC_QUERYBUF, &buf) == -1)
			WARNING_RETURN(FALSE, "Unable to query buffer %d for device: %s", i, device_name);
		
		if ((data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset)) == MAP_FAILED)
			WARNING_RETURN(FALSE, "Unable to map memory for buffer %d on device: %s", i, device_name);
		
		self->priv->buffers[self->priv->num_buffers].length = buf.length;
		self->priv->buffers[self->priv->num_buffers].data = data;
		self->priv->num_buffers++;

		if (ioctl_ex(fd, VIDIOC_QBUF, &buf) == -1)
			WARNING_RETURN(FALSE, "Unable to queue buffer %d on device %s", i, device_name);
	}
	return TRUE;
}


/* C920 Video Device - Start
 * -------------------------
 * Start the device. If the device is already started, do nothing and increment
 * the start_count. When stop is called it will decrement this value, so that
 * multiple references to the device can be obtained, and as long as one reference
 * has started and not stopped, the device keeps streaming
 */
gboolean c920_video_device_start(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	g_print("Starting device\n");	
	g_mutex_lock(&mutex);
	if (self->priv->start_count)
	{
		g_mutex_unlock(&mutex);
		return TRUE;
	}

	g_debug("Starting device %s", self->priv->device_name);

	// open the device
	if (!c920_video_device_open(self)) 
	{
		return FALSE;
	}

	if (!c920_video_device_setup_stream(self))
	{
		c920_video_device_close(self);
		return FALSE;
	}

	if (!c920_video_device_memory_map(self))
	{
		c920_video_device_close(self);
		return FALSE;
	}

	if (ioctl_ex(self->priv->fd, VIDIOC_STREAMON, &type) == -1)
	{
		g_warning("Unable to start device: %s", self->priv->device_name);
		c920_video_device_close(self);
		return FALSE;
	}

	//g_idle_add_full(100, c920_video_device_process, self, NULL);

	self->priv->start_count++;
	g_mutex_unlock(&mutex);	
	g_print("Started Video Device\n");
	
	self->priv->thread = g_thread_new("video device process", c920_video_device_process, self);

	return TRUE;
}


/* C920 Video Device - Stop
 * ------------------------
 * Stop the device, once the start_count hits 0
 * Other instances may hold a ref to the video and have started it but not stopped it
 * in which case we want to continue streaming
 */
gboolean c920_video_device_stop(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	g_print("Stopping Video Device\n");
	g_mutex_lock(&mutex);
	if (self->priv->start_count != 1) 
	{
		self->priv->start_count--;
		g_mutex_unlock(&mutex);
		return TRUE;
	}

	self->priv->start_count = 0;
	g_mutex_unlock(&mutex);
	
	g_thread_join(self->priv->thread);
	g_print("Stopped Device\n");
	
	if (self->priv->fd)
	{
		// should be silently ignored if device already stopped	
		if (ioctl_ex(self->priv->fd, VIDIOC_STREAMOFF, &type) == -1)
		{
        		g_warning("Unable to stop device: %s", self->priv->device_name);
		}
	}

	return c920_video_device_close(self);
}


/* C920 Video Device - Process
 * ---------------------------
 * This function is attached to the main loop and will run on idle (high prio idle)
 * It will try to dequeue a buffer and send it to all the callbacks in the hashtable
 */
gpointer c920_video_device_process(gpointer userdata)
{
	C920VideoDevice *self = (C920VideoDevice*)userdata;
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;

	fd_set fds;
	struct timeval tv;
	struct v4l2_buffer buf = {0};
	int fd = self->priv->fd;
	const char* device_name = self->priv->device_name;
	
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	while (TRUE)
	{
		usleep(5000);
		g_mutex_lock(&mutex);
		if (!C920_IS_VIDEO_DEVICE(self)) 
		{
			g_mutex_unlock(&mutex);
			return NULL;
		}
		if (!self->priv->start_count) 
		{
			g_mutex_unlock(&mutex);
			return NULL;
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		// select the device
		switch (select(fd+1, &fds, NULL, NULL, &tv))
		{
			case -1:
			{
				if (errno == EINTR) 
				{	
					g_mutex_unlock(&mutex);	
					continue;
				}
				else 
				{
					g_warning("Unable to select device: %s", device_name);
					g_mutex_unlock(&mutex); usleep(1000000); continue;
				}
			}
			case 0:
			{
				g_warning("Timeout occurred while selecting device: %s", device_name);
				g_mutex_unlock(&mutex); usleep(1000000); continue;
			}
		}
	
		// dequeue a buffer
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl_ex(fd, VIDIOC_DQBUF, &buf) == -1)
		{
			if (errno == EAGAIN) 
			{
				g_mutex_unlock(&mutex);
				continue;
			}
			else 
			{
				g_warning( "Error while trying to dequeue buffer for device: %s", device_name);
				g_mutex_unlock(&mutex); usleep(1000000); continue;
			}
		}
		if (buf.index >= self->priv->num_buffers)
		{
			g_warning("Invalid buffer index for device: %s", device_name);
			g_mutex_unlock(&mutex); usleep(1000000); continue;
		}
	
		if (self->priv->dump_file)
		{
			fwrite(self->priv->buffers[buf.index].data, 1, buf.bytesused, self->priv->dump_file);
			fflush(self->priv->dump_file);
		}
	
		GList *list = g_hash_table_get_values(self->priv->hashtable); 
		GList *node = list;

		while (node && node->data) 
		{
			struct c920_callback *cb = (struct c920_callback*)node->data;
			cb->cb(self->priv->buffers[buf.index].data, buf.bytesused, cb->userdata);
			node = node->next;
		}
	
		g_list_free(list);

		// queue the buffer again
		if (ioctl_ex(fd, VIDIOC_QBUF, &buf) == -1)
		{
			g_warning("Unable to queue buffer for device: %s", device_name);
			g_mutex_unlock(&mutex); usleep(1000000); continue;
		}
		g_mutex_unlock(&mutex);
	}
}
