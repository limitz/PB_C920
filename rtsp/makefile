INCLUDES = `pkg-config --cflags glib-2.0 gstreamer-0.10`
LIBRARIES = `pkg-config --libs glib-2.0 gstreamer-0.10` -lgstrtspserver-0.10
OBJECTS = c920_rtsp_server.o c920_rtsp_media_mapping.o c920_rtsp_media_factory.o c920_live_src.o c920_video_device.o c920_device_manager.o
CCPARAMS = -02 -Wall -marm

c920_service: c920_service.c $(OBJECTS)
	cc $(CCPARAMS) c920_service.c -o $@ $(OBJECTS) $(INCLUDES) $(LIBRARIES)

c920_rtsp_server.o: c920_rtsp_server.c
	cc -c $(CCPARAMS) c920_rtsp_server.c -o $@ $(INCLUDES)

c920_rtsp_media_mapping.o: c920_rtsp_media_mapping.c 
	cc -c $(CCPARAMS) c920_rtsp_media_mapping.c -o $@ $(INCLUDES) 

c920_rtsp_media_factory.o: c920_rtsp_media_factory.c 
	cc -c $(CCPARAMS) c920_rtsp_media_factory.c -o $@ $(INCLUDES)

c920_live_src.o: c920_live_src.c
	cc -c $(CCPARAMS) c920_live_src.c -o $@ $(INCLUDES)

c920_video_device.o: c920_video_device.c
	cc -c $(CCPARAMS) c920_video_device.c -o $@ $(INCLUDES)

c920_device_manager.o: c920_device_manager.c
	cc -c $(CCPARAMS) c920_device_manager.c -o $@ $(INCLUDES)

clean:
	rm c920_service *.o

