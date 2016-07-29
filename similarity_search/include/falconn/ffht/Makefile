CC = gcc
CFLAGS = -O3 -march=native -std=c99 -pedantic -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
CXX = g++
CXXFLAGS = -O3 -march=native -std=c++11 -Wall

.PHONY: all clean

all: best_chunk test fht.o

best_chunk: best_chunk.cpp fht.h fht.o
	$(CXX) $(CXXFLAGS) best_chunk.cpp -o best_chunk fht.o

test: test.cpp fht.h fht.o
	$(CXX) $(CXXFLAGS) test.cpp -o test fht.o

fht.o: fht.h fht_impl.h fht.c
	$(CC) $(CFLAGS) -c fht.c -o fht.o

clean:
	rm -f fht.o best_chunk test
