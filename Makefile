CC=g++
CFLAGS=-g $(shell pkg-config --cflags opencv) -I/usr/include/opencv2 -L/usr/X11R6/lib/
LDFLAGS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs $(shell pkg-config --libs x11 libcurl)
SRC=main.cpp
BIN=image_to_mouse

all:
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)
clean:
	rm *.o
