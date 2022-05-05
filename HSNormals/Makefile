CC = g++

CFLAGS = -DGL_GLEXT_LEGACY -DGL_GLEXT_PROTOTYPES
LIBS = -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lGLU -ljpeg -lpng

TARGET = main
OBJS = main.o glapp.o pbuffer.o mesh.o texture.o font.o utile.o metaballs.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) *.o
