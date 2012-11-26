#include "c920_video_device.h"

#define C920_VIDEO_DEVICE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), C920_TYPE_VIDEO_DEVICE, C920VideoDevicePrivate))


G_DEFINE_TYPE(C920VideoDevice, c920_video_device, G_TYPE_OBJECT);


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

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL };

struct c920_buffer
{
        void *data;
        size_t length;
};

struct _C920VideoDevicePrivate
{
	// properties
	gchar *device_name, *dump_file_name;
	guint width, height, fps;
	C920VideoBufferFunc cb;
	gpointer cb_userdata;
	
	// device io
	int fd;
	gboolean started;
	struct c920_buffer *buffers;
	size_t num_buffers;
	FILE *dump_file;
};


static void c920_video_device_class_init(C920VideoDeviceClass*);
static void c920_video_device_init(C920VideoDevice*);
static void c920_video_device_get_property(GObject*, guint, GValue*, GParamSpec*);
static void c920_video_device_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void c920_video_device_dispose(GObject*);
static void c920_video_device_finalize(GObject*);
static void c920_video_device_set_device_name(C920VideoDevice*, const gchar*);

static gboolean c920_video_device_process(gpointer userdata);

static int ioctl_ex(int fh, int request, void* arg)
{
	int r;
	do { r = ioctl(fh, request, arg); } while (r == -1 && EINTR == errno);
	return r;
}

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
			G_PARAM_READWRITE);

	obj_properties[PROP_HEIGHT] =
		g_param_spec_uint(
			"height",
			"Frame height",
			"Set the frame height",
			0, 1080, 720,
			G_PARAM_READWRITE);

	obj_properties[PROP_FPS] = 
		g_param_spec_uint(
			"fps",
			"Frames per second",
			"Set the frames per second",
			1, 30, 25,
			G_PARAM_READWRITE);

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


static void c920_video_device_init(C920VideoDevice *self)
{
	self->priv = C920_VIDEO_DEVICE_GET_PRIVATE(self);
}

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

static void c920_video_device_set_property(GObject *obj, guint pidx, const GValue *value, GParamSpec *spec)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);

	/* todo: mutex this */

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

static void c920_video_device_dispose(GObject *obj)
{
	C920VideoDevice *self __unused = C920_VIDEO_DEVICE(obj);
	/* nothing to do for now */
	G_OBJECT_CLASS(c920_video_device_parent_class)->dispose(obj);
}

static void c920_video_device_finalize(GObject *obj)
{
	C920VideoDevice *self = C920_VIDEO_DEVICE(obj);
	g_free(self->priv->device_name);
	g_free(self->priv->dump_file_name);

	if (self->priv->started) c920_video_device_stop(self);
	
	G_OBJECT_CLASS(c920_video_device_parent_class)->finalize(obj);
}


// PROPERTIES

void c920_video_device_set_width(C920VideoDevice *self, guint width)
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

void c920_video_device_set_height(C920VideoDevice *self, guint height)
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

void c920_video_device_set_fps(C920VideoDevice *self, guint fps)
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
	if (self->priv->dump_file) fclose(self->priv->dump_file);
	if (name) self->priv->dump_file = fopen(name, "wb");
}

const gchar* c920_video_device_get_dump_file_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->dump_file_name;
}

void c920_video_device_set_callback(C920VideoDevice *self, C920VideoBufferFunc callback, gpointer userdata)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	self->priv->cb = callback;
	self->priv->cb_userdata = userdata;
}

static void c920_video_device_set_device_name(C920VideoDevice *self, const gchar* device_name)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(device_name != NULL);

	g_free(self->priv->device_name);
	self->priv->device_name = g_strdup(device_name);
}

const gchar* c920_video_device_get_device_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->device_name;
}


// DEVICE IO

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

// Set up device stream
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

// Set up device memory mapping
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

gboolean c920_video_device_start(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	g_debug("Starting device %s", self->priv->device_name);

	c920_video_device_stop(self);

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

	g_idle_add_full(100, c920_video_device_process, self, NULL);

	return TRUE;
}

gboolean c920_video_device_stop(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

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

gboolean c920_video_device_process(gpointer userdata)
{
	C920VideoDevice* self = (C920VideoDevice*)userdata;
	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;

	fd_set fds;
	struct timeval tv;
	struct v4l2_buffer buf = {0};
	int fd = self->priv->fd;
	const char* device_name = self->priv->device_name;
	
	if (fd == 0) return FALSE;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// select the device
	switch (select(fd+1, &fds, NULL, NULL, &tv))
	{
		case -1:
		{
			if (errno == EINTR) return TRUE;
			else 
			{
				c920_video_device_stop(self);
				WARNING_RETURN(FALSE, "Unable to select device: %s", device_name);
			}
		}
		case 0:
		{
			c920_video_device_stop(self);
			WARNING_RETURN(FALSE, "Timeout occurred while selecting device: %s", device_name);
		}
	}
	
	// dequeue a buffer
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl_ex(fd, VIDIOC_DQBUF, &buf) == -1)
	{
		if (errno == EAGAIN) return TRUE;
		else 
		{
			c920_video_device_stop(self);
			WARNING_RETURN(FALSE, "Error while trying to dequeue buffer for device: %s", device_name);
		}
	}
	if (buf.index >= self->priv->num_buffers)
	{
		c920_video_device_stop(self);
		WARNING_RETURN(FALSE, "Invalid buffer index for device: %s", device_name);
	}

	if (self->priv->dump_file)
	{
		fwrite(self->priv->buffers[buf.index].data, 1, buf.bytesused, self->priv->dump_file);
		fflush(self->priv->dump_file);
	}

	if (self->priv->cb) self->priv->cb(self->priv->buffers[buf.index].data, buf.bytesused, self->priv->cb_userdata);

	//if (_process_cb) _process_cb(_buffers[buffer.index].data, buffer.bytesused, _userdata);

	// queue the buffer again
	if (ioctl_ex(fd, VIDIOC_QBUF, &buf) == -1)
	{
		c920_video_device_stop(self);
		WARNING_RETURN(FALSE, "Unable to queue buffer for device: %s", device_name);
	}

	return TRUE;
}
