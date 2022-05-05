CC = gcc
CXX = g++

CFLAGS = -Wall -O3
LDFLAGS = -L/usr/X11R6/lib

TARGET = SubsurfaceScattering
DEPEND = .depend

INCLUDE =
SRCS = main.cpp

INCLUDE += -Iframework
SRCS += framework/GLApp.cpp \
	framework/GLExt.cpp \
	framework/RenderTarget.cpp \
	framework/MathLib.cpp \
	framework/Mesh.cpp \
	framework/Shader.cpp \
	framework/Image.cpp \
	framework/Texture.cpp \
	framework/Font.cpp

OBJS = $(SRCS:.cpp=.o)

LIBS = -lX11 -lXxf86vm -lGL -lGLU -ljpeg -lpng -lz

$(DEPEND):
	$(CC) -MM $(INCLUDE) $(CFLAGS) $(SRCS) > $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(DEPEND) $(OBJS)

-include $(DEPEND)
