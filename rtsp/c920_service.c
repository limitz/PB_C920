#include "c920_global.h"
#include "c920_rtsp_server.h"
#include "c920_video_device.h"
#include "c920_device_manager.h"

int main()
{
	C920RtspServer 		*rtsp_server;
	C920VideoDevice 	*video;
	C920DeviceManager 	*device_manager;
	GMainLoop 		*main_loop;

	g_type_init();
	gst_init(NULL, NULL);

	main_loop = g_main_loop_new(NULL, FALSE);
	rtsp_server = g_object_new(C920_TYPE_RTSP_SERVER, NULL);
	device_manager = g_object_new(C920_TYPE_DEVICE_MANAGER, NULL);

	video = g_object_new(
		C920_TYPE_VIDEO_DEVICE, 
		"device-name", "/dev/video0",
		"width", 1280,
		"height", 720,
		"fps", 20,
		"dump-file-name", "video0.h264", 
		NULL);

	c920_device_manager_add_device(
		device_manager,
		c920_video_device_get_device_name(video),
		video);

	g_object_unref(video);

	c920_rtsp_server_set_address(rtsp_server, "192.168.2.1");
	c920_rtsp_server_set_service(rtsp_server, "1935");
	c920_rtsp_server_attach(rtsp_server);

	if (!rtsp_server) exit(EXIT_FAILURE);

	g_main_loop_run(main_loop);

	g_object_unref(device_manager);
	g_object_unref(rtsp_server);

	return 0;
}
