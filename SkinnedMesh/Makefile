CC = cc

LDFLAGS = -L/usr/X11R6/lib
LIBS = -lglut -lGL -lGLU -lX11 -lXext -lXmu -lXi -lm

TARGET = skinnedmesh
OBJS = main.o mathlib.o loadtga.o loadjpeg.o skinnedmesh.o

.c.o:
	$(CC) -Wall -O2 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) $(OBJS)
