#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

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

#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264 v4l2_fourcc('H', '2', '6', '4')
#endif

#ifdef DEBUG
#define INFO(fmt, ...) printf(fmt "\n", ## __VA_ARGS__)
#else
#define INFO(fmt, ...) {}
#endif

typedef int (*cap_buffer_cb)(void* data, size_t length, void* userdata);

class cap_exception_t
{
	private: int _errno;
	private: char _message[1024];

	public: cap_exception_t(const char* fmt, ...)
	{
		_errno = errno;

		va_list args;
		va_start(args, fmt);
		vsprintf(_message, fmt, args);
		va_end(args); 
	}

	public: const char* message() const { return _message; }
	public: int error() const { return _errno; }
};

class cap_device_t
{
	private: bool   _is_playing;
	private: char* 	_device_name;
	private: int 	_fd;
	private: size_t	_num_buffers;
	private: struct _buffer { void* data; size_t length; };
	private: _buffer* _buffers;
	private: cap_buffer_cb _process_cb;
	private: void* _userdata;
	
	public: cap_device_t(const char* device_name, size_t width, size_t height, size_t fps, cap_buffer_cb cb, void* userdata)
	{
		struct stat st;
		v4l2_capability cap;
		v4l2_cropcap cropcap;
		v4l2_crop crop;
		v4l2_format fmt; 
		v4l2_requestbuffers req;
		v4l2_streamparm parm;

		size_t min;

		_device_name = 0;
		_is_playing = false;

		memset(&cropcap, 0, sizeof(cropcap));
		memset(&crop, 0, sizeof(crop));
		memset(&fmt, 0, sizeof(fmt));
		memset(&req, 0, sizeof(req));
		memset(&parm, 0, sizeof(parm));


		INFO("Identifying device %s", device_name);
		////---------------------------------------
		if (stat(device_name, &st) == -1) 
			throw cap_exception_t("unable to identify device %s", device_name);


		INFO("Testing to see if %s is a device", device_name);
		////-------------------------------------------------
		if (!S_ISCHR(st.st_mode)) 
			throw cap_exception_t("%s is not a device", device_name);

		INFO("Opening device %s as RDWR | NONBLOCK",device_name);
		////-----------------------------------------------------
		if ((_fd = open(device_name, O_RDWR | O_NONBLOCK, 0)) == -1)
			throw cap_exception_t("cannot open device %s", device_name);


		INFO("Querying V4L2 capabilities for device %s", device_name);
		////----------------------------------------------------------
		if (ioctl_ex(_fd, VIDIOC_QUERYCAP, &cap) == -1)
		{
			if (errno == EINVAL) throw cap_exception_t("%s is not a valid V4L2 device", device_name);
			else throw cap_exception_t("error in ioctl VIDIOC_QUERYCAP");		
		}


		INFO("Testing if device %s is a streaming capture device", device_name);
		////--------------------------------------------------------------------
		if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
			throw cap_exception_t("%s is not a capture device", device_name);

		if (!(cap.capabilities & V4L2_CAP_STREAMING))
			throw cap_exception_t("%s is not a streaming device", device_name);


		INFO("Trying to set crop rectange for device %s", device_name);
		////-----------------------------------------------------------
		cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl_ex(_fd, VIDIOC_CROPCAP, &cropcap) == 0)
		{
			crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			crop.c = cropcap.defrect;
			if (ioctl_ex(_fd, VIDIOC_S_CROP, &crop) == -1)
				INFO("W: Unable to set crop for device %s", device_name);
						
		} 
		else INFO("W: Unable to get crop capabilities for device %s", device_name);
		

		INFO("Setting video format to H.264 (w:%d, h:%d) for device %s", width, height, device_name);
		////-----------------------------------------------------------------------------------------
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = width;
		fmt.fmt.pix.height = height;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
		fmt.fmt.pix.field = V4L2_FIELD_ANY;

		if (ioctl_ex(_fd, VIDIOC_S_FMT, &fmt) == -1) 
			throw cap_exception_t("error in ioctl VIDIOC_S_FMT");

		// in case of buggy driver
		min = fmt.fmt.pix.width * 2;
		if (fmt.fmt.pix.bytesperline < min) fmt.fmt.pix.bytesperline = min;
		min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
		if (fmt.fmt.pix.sizeimage < min) fmt.fmt.pix.sizeimage = min;

		INFO("Video size set to: (w:%d, h:%d)", fmt.fmt.pix.width, fmt.fmt.pix.height);
		INFO("Video frame size set to: %d", fmt.fmt.pix.sizeimage);


		INFO("Getting video stream parameters for device %s", device_name);
		////---------------------------------------------------------------
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl_ex(_fd, VIDIOC_G_PARM, &parm) == -1)
			throw cap_exception_t("unable to get stream parameters for %s", device_name);

		INFO("Time per frame was: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = fps;
		INFO("Time per frame set: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);		

		if (ioctl_ex(_fd, VIDIOC_S_PARM, &parm) == -1)
			throw cap_exception_t("unable to set stream parameters for %s", device_name);

		INFO("Time per frame now: %d/%d", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);


		INFO("Initializing MMAP for device %s", device_name);
		////-------------------------------------------------
		req.count = 4;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		
		if (ioctl_ex(_fd,VIDIOC_REQBUFS, &req) == -1)
		{
			if (errno == EINVAL) throw cap_exception_t("%s does not support MMAP", device_name);
			else throw cap_exception_t("error in ioctl VIDIOC_REQBUFS");
		}

		INFO("Device %s can handle %d memory mapped buffers", device_name, req.count);
		if (req.count < 2) throw cap_exception_t("insufficient memory on device %s", device_name);
		
		INFO("Allocating %d buffers to map", req.count);
		_buffers = (_buffer*) calloc(req.count, sizeof(_buffer));
		if (!_buffers) throw cap_exception_t("out of memory");
		
		for (_num_buffers = 0; _num_buffers < req.count; _num_buffers++)
		{
			struct v4l2_buffer buf = {0};
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = _num_buffers;
			
			if (ioctl_ex(_fd, VIDIOC_QUERYBUF, &buf) == -1)
				throw cap_exception_t("error in ioctl VIDIOC_QUERYBUF");


			INFO("Mapping buffer %d", _num_buffers);
			_buffers[_num_buffers].length = buf.length;
			_buffers[_num_buffers].data = mmap(
				NULL, 
				buf.length, 
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				_fd, buf.m.offset);

			if (_buffers[_num_buffers].data == MAP_FAILED)
				throw cap_exception_t("mmap failed");
		}

		INFO("Queueing %d buffers for device %s", _num_buffers, _device_name);
                ////------------------------------------------------------------------
                for (size_t i=0; i<_num_buffers; i++)
                {
                        struct v4l2_buffer buf = {0};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        INFO("Queueing buffer %d", i);
                        if (ioctl_ex(_fd, VIDIOC_QBUF, &buf) == -1)
                                throw cap_exception_t("error in ioctl VIDIOC_QBUF");
                }


		// copy the device name so we can use it in error messages
		_device_name = (char*) malloc(strlen(device_name)+1);
		strcpy(_device_name, device_name);

		// set the callback
		_process_cb = cb;
		_userdata = userdata;

		INFO("Done with setup of device %s", device_name);
	}

	public: ~cap_device_t()
	{
		if (_is_playing) stop();


		INFO("Destroying memory mapped buffers for device %s", _device_name);
		////-----------------------------------------------------------------
		for (size_t i=0; i<_num_buffers; i++)
		{
			INFO("Unmapping buffer %d", i);
			if (munmap(_buffers[i].data, _buffers[i].length) == -1)
				throw cap_exception_t("Unable to unmap buffer %d", i);
		}
		

		INFO("Closing device %s", _device_name);
		////------------------------------------
		if (close(_fd) == -1) 
			throw cap_exception_t("Unable to close device %s", _device_name);

		if (_device_name) free(_device_name);
	}

	// Stop the capture device
	public: void stop()
        {
		if (!_is_playing) return;
		_is_playing = false;

		INFO("Stopping device %s", _device_name);
		////-------------------------------------
                enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (ioctl_ex(_fd, VIDIOC_STREAMOFF, &type) == -1)
                        throw cap_exception_t("error in ioctl VIDIOC_STREAMOFF");
        }


	// Start the capture device
	public: void start()
	{
		if (_is_playing) return;
		_is_playing = true;

		INFO("Starting device %s", _device_name);
		////------------------------------------
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl_ex(_fd, VIDIOC_STREAMON, &type) == -1)
			throw cap_exception_t("error in ioctl VIDIOC_STREAMON");
	}

	// Process a single frame from the capture stream, call this in a loop
	public: int process()
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(_fd, &fds);
		
		timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		// select the device
		switch (select(_fd+1, &fds, NULL, NULL, &tv))
		{
			case -1:
			{
				if (errno == EINTR) return 1;
				else throw cap_exception_t("Could not select device %s", _device_name);
			}
			case 0:
			{
				throw cap_exception_t("timeout occurred while selecting device %s", _device_name);
			}
		}
		
		// dequeue a buffer
		v4l2_buffer buffer = {0};
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;

		if (ioctl_ex(_fd, VIDIOC_DQBUF, &buffer) == -1)
		{
			if (errno == EAGAIN) return 1;
			else throw cap_exception_t("error in ioctl VIDIOC_DQBUF");
		}

		assert(buffer.index < _num_buffers);

		int r = 0;
		if (_process_cb) r = _process_cb(_buffers[buffer.index].data, buffer.bytesused, _userdata);

		// queue the buffer again
		if (ioctl_ex(_fd, VIDIOC_QBUF, &buffer) == -1)
			throw cap_exception_t("error in ioctl VIDIOC_QBUF");

		return r;
	}

	private: static int ioctl_ex(int fh, int request, void* arg)
	{
		int r;
		do { r = ioctl(fh, request, arg); } while (r == -1 && EINTR == errno);
		return r;
	}
};
#endif
