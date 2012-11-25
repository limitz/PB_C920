#include "c920_global.h"
#include "c920_rtsp_server.h"

int main()
{
	C920RtspServer *rtsp_server;

	g_type_init();
	gst_init(NULL, NULL);

	rtsp_server = g_object_new(C920_TYPE_RTSP_SERVER, NULL);
	if (!rtsp_server) exit(EXIT_FAILURE);

	getchar();

	g_object_unref(rtsp_server);

	return 0;
}
