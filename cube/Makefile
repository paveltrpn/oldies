CC       = cc
CFLAGS   = -Wall 
LFLAGS 	 = -lGL -lglut

SRCS := cube.c
OBJS := $(patsubst %.c,%.o,$(SRCS))

.PHONY: clean all

all: $(OBJS) main

cube.o: cube.cpp
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJS)
	$(CC) $(OBJS) -o main $(LFLAGS)

clean:
	rm -f *.o main