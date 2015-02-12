EXEC = sqfd_feature
LIBS = `pkg-config --libs --cflags opencv` -lpthread
CC = g++
CPPFLAGS = -I/usr/local/include/eigen3 -I/usr/include/eigen3 -Wall -std=c++11 -Ofast

OBJS = $(patsubst %.cc, %.o, $(wildcard *.cc))
HDRS = $(wildcard *.h)

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS) $(HDRS)
	$(CC) $(CPPFLAGS) $(OBJS) $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f $(EXEC)

