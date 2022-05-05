CC       = g++
CFLAGS   = -Wall 
LFLAGS 	 = -lstdc++ -lGL -lglut

SRCS := marching.cpp
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

.PHONY: clean all

all: $(OBJS) main

marching.o: marching.cpp
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJS)
	$(CC) $(OBJS) -o main $(LFLAGS)

clean:
	rm -f *.o main