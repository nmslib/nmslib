# Set these variables if you installed the library to a non-standard location
#NON_METRIC_SPACE_LIBRARY=/home/leo/NonMetrLibDebug
NON_METRIC_SPACE_LIBRARY=$(HOME)/NonMetrLibRelease
NON_METRIC_SPACE_LIBRARY_INC=$(NON_METRIC_SPACE_LIBRARY)/include
NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/lib

LIB_GSL=-lgsl -lgslcblas -llshkit

CXXFLAGS += -I$(NON_METRIC_SPACE_LIBRARY_INC) 
# Enable C++11
CXXFLAGS += -std=c++11
# Debug flags
#CXXFLAGS += -g
CXXFLAGS += -g -O3

BIN=test_app

all: $(BIN)

clean:
	rm -f *.o $(BIN)

# Note -pthread: this enables threads!!!
test_app: test_app.o data.o *.h makefile
	$(CXX) -o$@ test_app.o data.o -lNonMetricSpaceLib -L$(NON_METRIC_SPACE_LIBRARY_LIB) $(LIB_GSL) -pthread

