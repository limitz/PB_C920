#ifndef C920_GLOBAL_H
#define C920_GLOBAL_H

#define DEBUG

#include <glib-object.h>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include <string.h>

#define C920_RTSP_HOST "192.168.2.1"
#define C920_RTSP_PORT "1935"

#define __unused __attribute__((unused))

#ifdef DEBUG
#define INFO(fmt, ...) g_print(fmt "\n", ## __VA_ARGS__)
#else
#define INFO(fmt, ...) {}
#endif

#endif
