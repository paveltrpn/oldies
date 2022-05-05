CC = cc

CFLAGS = `sdl-config --cflags`
LIBS = `sdl-config --libs` -L/usr/X11R6/lib -lGL -lGLU -ljpeg -lm

TARGET = bump
OBJS = mathlib.o main.o font.o texture.o load3ds.o bump_mesh.o

.c.o:
	$(CC) $(CFLAGS) -Wall -O2 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) $(OBJS)
