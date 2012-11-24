#define DEBUG
#include "capture.h"

static FILE* fp;

int process_frame(void* data, size_t length)
{
	static long bytes = 0;

	fwrite(data, 1, length, fp);
	fflush(fp);
	bytes+=length;
	return bytes > 4*1024*1024 ? 0 : 1;
}

int main()
{
	cap_device_t* camera = 0;
	fp = fopen("test.h264", "wb");
	if (!fp) exit(-1);

	try
	{
		camera = new cap_device_t("/dev/video0", 1280, 720, 20, process_frame);
	}
	catch (cap_exception_t &e)
	{
		printf("%s", e.message());
		if (e.error())
		{
			printf(" (%d: %s)", e.error(), strerror(e.error()));
		}
		printf("\n");
	}

	camera->start();

	while (camera->process());

	camera->stop();
	fclose(fp);
	if (camera) delete camera;

	return 0;
}
