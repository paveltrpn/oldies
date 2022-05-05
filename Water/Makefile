CC = g++

CFLAGS = `sdl-config --cflags`
LIBS = `sdl-config --libs` -L/usr/X11R6/lib -lGL -lGLU -ljpeg

TARGET = main
OBJS = main.o mesh.o font.o texture.o object.o

.cpp.o:
	g++ $(CFLAGS) -Wall -O2 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)
