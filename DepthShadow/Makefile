CC = g++

CFLAGS = 
LIBS = -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lGLU -ljpeg 

TARGET = main
OBJS = main.o mesh.o glapp.o mesh.o font.o texture.o glew.c

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O2 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

main.o: glapp.h
glapp.o: glapp.h

clean:
	rm -f $(TARGET) *.o
