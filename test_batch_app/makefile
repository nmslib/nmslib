#NON_METRIC_SPACE_LIBRARY=/home/leo/NonMetrLibDebug
NON_METRIC_SPACE_LIBRARY=../similarity_search
NON_METRIC_SPACE_LIBRARY_INC=$(NON_METRIC_SPACE_LIBRARY)/include
NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/release
#NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/debug

#LIB_GSL=-lgsl -lgslcblas -llshkit

CXXFLAGS += -I$(NON_METRIC_SPACE_LIBRARY_INC) 
# Enable C++11
CXXFLAGS += -std=c++11
# OMP flags
CXXFLAGS += -fopenmp
# Debug flags
#CXXFLAGS += -g
CXXFLAGS += -g -O3

BIN=test_batch_mod

all: $(BIN)

clean:
	rm -f *.o $(BIN)

# Note -pthread: this enables threads!!!
test_batch_mod: test_batch_mod.o makefile $(NON_METRIC_SPACE_LIBRARY_LIB)/libNonMetricSpaceLib.a
	$(CXX) -o$@ test_batch_mod.o -lNonMetricSpaceLib -L$(NON_METRIC_SPACE_LIBRARY_LIB) $(LIB_GSL) -pthread -fopenmp

