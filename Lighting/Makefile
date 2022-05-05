CC = g++

CFLAGS = 
LDFLAGS = -L/usr/X11R6/lib

LIBS = -lX11 -lXxf86vm -lGL -lGLU -ljpeg -lpng -lz

TARGET = main
DEPEND = .depend

OBJS = main.o glapp.o glext.o font.o texture.o mesh.o particles.o shader.o spline.o

$(DEPEND):
	gcc -MM *.cpp > $@

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(TARGET) $(DEPEND) *.o

-include $(DEPEND)
