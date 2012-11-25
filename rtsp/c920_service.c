#include "c920_global.h"
#include "c920_rtsp_server.h"

int main()
{
	C920RtspServer 	*rtsp_server;
	GMainLoop 	*main_loop;

	g_type_init();
	gst_init(NULL, NULL);

	main_loop = g_main_loop_new(NULL, FALSE);
	rtsp_server = g_object_new(C920_TYPE_RTSP_SERVER, NULL);
	if (!rtsp_server) exit(EXIT_FAILURE);

	g_main_loop_run(main_loop);

	g_object_unref(rtsp_server);

	return 0;
}
