NON_METRIC_SPACE_LIBRARY=../similarity_search/
NON_METRIC_SPACE_LIBRARY_INC=$(NON_METRIC_SPACE_LIBRARY)/include
NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/release

LIBS= -lthrift -lNonMetricSpaceLib -lgsl -lgslcblas -llshkit -lboost_program_options

CXXFLAGS += -I$(NON_METRIC_SPACE_LIBRARY_INC)  -I./gen-cpp/
# Enable C++11
CXXFLAGS += -std=c++11
# OMP flags
CXXFLAGS += -fopenmp
# Debug flags
#CXXFLAGS += -g
CXXFLAGS += -g -O3

BIN=query_server

all: $(BIN)

clean:
	rm -f *.o gen-cpp/*.o $(BIN)

THRIFT_SRC=$(wildcard gen-cpp/*.cpp)
THRIFT_OBJ=$(patsubst %.cpp,%.o,$(THRIFT_SRC))

# Note -pthread: this enables threads!!!
query_server:  $(THRIFT_OBJ) QueryService_server.o gen-cpp/*.h makefile
	$(CXX) -o$@  $(THRIFT_OBJ) QueryService_server.o -L/usr/local/lib -L$(NON_METRIC_SPACE_LIBRARY_LIB) $(LIBS) -pthread -fopenmp 

