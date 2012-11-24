capture: capture.cpp capture.h
	g++ -O2 -Wall -marm `pkg-config --cflags --libs libv4l2` capture.cpp -o capture

clean:
	rm capture capture.o
