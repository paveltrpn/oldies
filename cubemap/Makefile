CC       = cc
CFLAGS   = 
LFLAGS 	 = -lglut -lGLU -lGL -lXext -lX11 -lm

SRCS := cm_demo.c cubemap.c tga.c trackball.c
OBJS := $(patsubst %.c,%.o,$(SRCS))

.PHONY: clean all

all: $(OBJS) cm_demo cubemap

cm_demo.o: cm_demo.c
	$(CC) $(CFLAGS) -c $< -o $@

cubemap.o: cubemap.c
	$(CC) $(CFLAGS) -c $< -o $@

tga.o: tga.c
	$(CC) $(CFLAGS) -c $< -o $@

trackball.o: trackball.c
	$(CC) $(CFLAGS) -c $< -o $@

cm_demo: cm_demo.o tga.o trackball.o
	$(CC) cm_demo.o tga.o trackball.o -o cm_demo $(LFLAGS)

cubemap: cubemap.o 
	$(CC) cubemap.o -o cubemap $(LFLAGS)

clean:
	rm -f *.o cm_demo cubemap