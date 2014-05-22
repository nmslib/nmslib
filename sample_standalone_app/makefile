# Set these variables if you installed the library to a non-standard location
NON_METRIC_SPACE_LIBRARY_INC=/home/leo/NonMetrLibDebug/include
NON_METRIC_SPACE_LIBRARY_LIB=/home/leo/NonMetrLibDebug/lib

LIB_GSL=-lgsl -lgslcblas -llshkit

CXXFLAGS += -I$(NON_METRIC_SPACE_LIBRARY_INC) 
# Enable C++11
CXXFLAGS += -std=c++11
# Debug flags
CXXFLAGS += -g

BIN=test_app

all: $(BIN)

clean:
	rm -f *.o $(BIN)

# Note -pthread: this enables threads!!!
test_app: test_app.o data.o *.h
	$(CXX) -o$@ test_app.o data.o -l NonMetricSpaceLib -L$(NON_METRIC_SPACE_LIBRARY_LIB) $(LIB_GSL) -pthread

