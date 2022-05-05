CC       = g++
CFLAGS   = -Wall 
LFLAGS 	 = -lstdc++ -lGL -lglut

SRCS := CIsoSurface.cpp Vectors.cpp
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

.PHONY: clean all

all: $(OBJS) main

ClsoSurface.o: CIsoSurface.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Vectors.o: Vectors.cpp
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJS)
	$(CC) $(OBJS) -o main $(LFLAGS)

clean:
	rm -f *.o main