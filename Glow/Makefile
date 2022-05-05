CC = g++

CFLAGS = 
LIBS = -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lGLU -ljpeg

TARGET = main
OBJS = main.o mesh.o glapp.o mesh.o font.o texture.o pbuffer.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) *.o
