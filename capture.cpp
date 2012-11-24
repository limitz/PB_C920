#define DEBUG
#define FILENAME "test.h264"
#define MB(x) (x*1024*1024)

#include "capture.h"

int process_frame(void* data, size_t length, void* userdata)
{
	static long bytes = 0;
	FILE* fp = (FILE*) userdata;
	
	fwrite(data, 1, length, fp);
	fflush(fp);

	bytes+=length;

	return bytes < MB(4) ? 1 : 0;
}

int main()
{
	FILE* fp = fopen("test.h264", "wb");
	if (!fp) 
	{
		fprintf(stderr, "Unable to open file for writing: %s", FILENAME);
		exit(EXIT_FAILURE);
	}

	try
	{
		cap_device_t* camera = new cap_device_t("/dev/video0", 1280, 720, 20, process_frame, fp);

		camera->start();
		while (camera->process()) /* keep running untill callback says it's done*/;
		camera->stop();

		delete camera;
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

	fclose(fp);

	return 0;
}
