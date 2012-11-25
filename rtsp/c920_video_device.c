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
	gchar *device_name, *dump_file_name;
	guint width, height, fps;
	
	int fd;
	gboolean started;
	struct c920_buffer *buffers;
	size_t num_buffers;
};


static void c920_video_device_class_init(C920VideoDeviceClass*);
static void c920_video_device_init(C920VideoDevice*);
static void c920_video_device_get_property(GObject*, guint, GValue*, GParamSpec*);
static void c920_video_device_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void c920_video_device_dispose(GObject*);
static void c920_video_device_finalize(GObject*);
static void c920_video_device_set_device_name(C920VideoDevice*, const gchar*);

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
			/* todo: update dump file on the fly */
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
	if (self->priv->fd) close(self->priv->fd);

	G_OBJECT_CLASS(c920_video_device_parent_class)->finalize(obj);
}

void c920_video_device_set_width(C920VideoDevice *self, guint width)
{
	g_return_if_fail(C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(width > 0 && width <= 1920);

	self->priv->width = width;
	
	g_debug("Setting frame width to %d for device %s", self->priv->width, self->priv->device_name);
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

	g_debug("Setting frame height to %d for device %s", self->priv->height, self->priv->device_name);
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
	g_debug("Setting fps to %d for device %s", self->priv->fps, self->priv->device_name);
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
	if (!name) self->priv->dump_file_name = NULL;
	else self->priv->dump_file_name = g_strdup(name);

	g_debug("Setting dump file to %s for device %s", self->priv->dump_file_name, self->priv->device_name);
}

const gchar* c920_video_device_get_dump_file_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->dump_file_name;
}

static void c920_video_device_set_device_name(C920VideoDevice *self, const gchar* device_name)
{
	int fd;
	struct stat st;
	struct v4l2_capability cap;

	g_return_if_fail(!C920_IS_VIDEO_DEVICE(self));
	g_return_if_fail(device_name != NULL);

	g_debug("Identifying device %s", device_name);
        if (stat(device_name, &st) == -1)
	{
                g_warning("unable to identify device %s", device_name);
		return;
	}

        g_debug("Testing to see if %s is a device", device_name);
        if (!S_ISCHR(st.st_mode))
	{
                g_warning("%s is not a device", device_name);
		return;
	}

        g_debug("Opening device %s as RDWR | NONBLOCK", device_name);
        if ((fd = open(device_name, O_RDWR | O_NONBLOCK, 0)) == -1)
	{
                g_warning("cannot open device %s", device_name);
		return;
	}

        g_debug("Querying V4L2 capabilities for device %s", device_name);
        if (ioctl_ex(fd, VIDIOC_QUERYCAP, &cap) == -1)
        {
                if (errno == EINVAL) g_warning("%s is not a valid V4L2 device", device_name);
                else g_warning("error in ioctl VIDIOC_QUERYCAP");
		close(fd);
		return;
        }

        g_debug("Testing if device %s is a streaming capture device", device_name);
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
                g_warning("%s is not a capture device", device_name);
		close(fd);
		return;
	}

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
                g_warning("%s is not a streaming device", device_name);
		close(fd);
		return;
	}

	self->priv->fd = fd;
	g_free(self->priv->device_name);
	self->priv->device_name = g_strdup(device_name);
}

const gchar* c920_video_device_get_device_name(C920VideoDevice *self)
{
	if (!C920_IS_VIDEO_DEVICE(self)) return NULL;
	return self->priv->device_name;
}

gboolean c920_video_device_start(C920VideoDevice *self)
{
	size_t min, i;

	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_streamparm parm;
	enum   v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;
		
	memset(&cropcap, 0, sizeof(cropcap));
	memset(&crop, 0, sizeof(crop));
	memset(&fmt, 0, sizeof(fmt));
	memset(&req, 0, sizeof(req));
	memset(&parm, 0, sizeof(parm));

	if (self->priv->started)
	{
		g_warning("trying to start a device that has already been started and not yet stopped");
		return FALSE;
	}

	g_debug("Trying to set crop rectange for device %s", self->priv->device_name);
	cropcap.type = type;
	if (ioctl_ex(self->priv->fd, VIDIOC_CROPCAP, &cropcap) == 0)
	{
		crop.type = type;
		crop.c = cropcap.defrect;
		if (ioctl_ex(self->priv->fd, VIDIOC_S_CROP, &crop) == -1) g_debug("Unable to set crop for device %s", self->priv->device_name);	
	}
	else g_warning("Unable to get crop rectangle for device %s", self->priv->device_name);
	

	g_debug("Setting video format to H.264 (w:%d, h:%d) for device %s", self->priv->width, self->priv->height, self->priv->device_name);
	fmt.type = type;
	fmt.fmt.pix.width = self->priv->width;
	fmt.fmt.pix.height = self->priv->height;
	fmt.fmt.pix.pixelformat = v4l2_fourcc('H', '2', '6', '4');
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	if (ioctl_ex(self->priv->fd, VIDIOC_S_FMT, &fmt) == -1)
	{
		g_warning("error in ioctl VIDIOC_S_FMT");
		return FALSE;
	}
	
	// in case of buggy driver
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min) fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min) fmt.fmt.pix.sizeimage = min;

	g_debug("Video size set to: (w:%d, h:%d)", fmt.fmt.pix.width, fmt.fmt.pix.height);
	g_debug("Video frame size set to: %d", fmt.fmt.pix.sizeimage);


	g_debug("Getting video stream parameters for device %s", self->priv->device_name);
	parm.type = type;
	if (ioctl_ex(self->priv->fd, VIDIOC_G_PARM, &parm) == -1)
	{
		g_warning("unable to get stream parameters for %s", self->priv->device_name);
		return FALSE;
	}

	g_debug("Time per frame was: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = self->priv->fps;
	g_debug("Time per frame set: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);		

	if (ioctl_ex(self->priv->fd, VIDIOC_S_PARM, &parm) == -1)
	{
		g_warning("unable to set stream parameters for %s", self->priv->device_name);
		return FALSE;
	}
	g_debug("Time per frame now: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);


	g_debug("Initializing MMAP for device %s", self->priv->device_name);
	req.count = 4;
	req.type = type;
	req.memory = V4L2_MEMORY_MMAP;
		
	if (ioctl_ex(self->priv->fd,VIDIOC_REQBUFS, &req) == -1)
	{
		if (errno == EINVAL) g_warning("%s does not support MMAP", self->priv->device_name);
		else g_warning("error in ioctl VIDIOC_REQBUFS");
		return FALSE;
	}

	g_debug("Device %s can handle %d memory mapped buffers", self->priv->device_name, req.count);
	if (req.count < 2) 
	{
		g_warning("insufficient memory on device %s", self->priv->device_name);
		return FALSE;
	}
		
	g_debug("Allocating %d buffers to map", req.count);
	self->priv->buffers = (struct c920_buffer*) calloc(req.count, sizeof(struct c920_buffer));
	if (!self->priv->buffers)
	{
		g_warning("out of memory");
		return FALSE;
	}
	
	for (self->priv->num_buffers = 0; self->priv->num_buffers < req.count; self->priv->num_buffers++)
	{
		struct v4l2_buffer buf = {0};
		buf.type = type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = self->priv->num_buffers;
			
		if (ioctl_ex(self->priv->fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			g_warning("error in ioctl VIDIOC_QUERYBUF");
			return FALSE;
		}

		g_debug("Mapping buffer %d", self->priv->num_buffers);
		self->priv->buffers[self->priv->num_buffers].length = buf.length;
		self->priv->buffers[self->priv->num_buffers].data = mmap(
			NULL, 
			buf.length, 
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			self->priv->fd, buf.m.offset);

		if (self->priv->buffers[self->priv->num_buffers].data == MAP_FAILED)
		{
			// todo: facilitate unmap of already mapped buffers
			g_warning("mmap failed");
			return FALSE;
		}
	}


	g_debug("Queueing %d buffers for device %s", self->priv->num_buffers, self->priv->device_name);
	for (i=0; i<self->priv->num_buffers; i++)
	{
		struct v4l2_buffer buf = {0};
		buf.type = type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		g_debug("Queueing buffer %d", i);
		if (ioctl_ex(self->priv->fd, VIDIOC_QBUF, &buf) == -1)
		{
			// todo: facilitate dequeue
			g_warning("error in ioctl VIDIOC_QBUF");
			return FALSE;
		}
	}

	g_debug("Starting device %s", self->priv->device_name);
	if (ioctl_ex(self->priv->fd, VIDIOC_STREAMON, &type) == -1)
	{
		g_warning("error in ioctl VIDIOC_STREAMON");
		return FALSE;
	}

	// attach proc to main loop

	self->priv->started = TRUE;
	return TRUE;
}

gboolean c920_video_device_stop(C920VideoDevice *self)
{
	size_t i;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (!C920_IS_VIDEO_DEVICE(self)) return FALSE;

	if (!self->priv->started)
	{
		g_warning("trying to stop a device that has not started");
		return FALSE;
	}
	self->priv->started = FALSE;

	g_debug("Stopping device %s", self->priv->device_name);
	if (ioctl_ex(self->priv->fd, VIDIOC_STREAMOFF, &type) == -1)
	{
        	g_warning("error in ioctl VIDIOC_STREAMOFF");	
	}

	g_debug("Destroying memory mapped buffers for device %s", self->priv->device_name);
	for (i=0; i<self->priv->num_buffers; i++)
	{
		g_debug("Unmapping buffer %d", i);
		if (munmap(self->priv->buffers[i].data, self->priv->buffers[i].length) == -1)
		{
			g_warning("Unable to unmap buffer %d", i);
		}
	}
	return TRUE;
}
