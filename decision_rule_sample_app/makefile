# Set these variables if you installed the library to a non-standard location
NON_METRIC_SPACE_LIBRARY=../similarity_search/
NON_METRIC_SPACE_LIBRARY_INC=$(NON_METRIC_SPACE_LIBRARY)/include
NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/release

LIB_ADD=-lgsl -lgslcblas -llshkit

CXXFLAGS += -I$(NON_METRIC_SPACE_LIBRARY_INC) 
# Enable C++11
CXXFLAGS += -std=c++11
# OMP flags
CXXFLAGS += -fopenmp
# Debug flags
#CXXFLAGS += -g
#CXXFLAGS += -g -O3
CXXFLAGS += -Ofast

BIN=sample_dec_rule

all: $(BIN)

clean:
	rm -f *.o $(BIN)

# Note -pthread: this enables threads!!!
sample_dec_rule: sample_dec_rule.o searchoracle_old.o makefile
	$(CXX) -o$@ sample_dec_rule.o searchoracle_old.o -lNonMetricSpaceLib -L$(NON_METRIC_SPACE_LIBRARY_LIB) $(LIB_ADD) -pthread -fopenmp

