CC       = cc
CFLAGS   = -Wall 
LFLAGS 	 = -lstdc++ -lGL -lglut

SRCS := glvolume.c drawing.c menus.c 
OBJS := $(patsubst %.c,%.o,$(SRCS))

.PHONY: clean all

all: $(OBJS) main

glvolume.o: glvolume.c
	$(CC) $(CFLAGS) -c $< -o $@

drawing.o: drawing.c
	$(CC) $(CFLAGS) -c $< -o $@

menus.o: menus.c
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJS)
	$(CC) $(OBJS) -o main $(LFLAGS)

clean:
	rm -f *.o main