PYTHON_VERSION=2.7
NON_METRIC_SPACE_LIBRARY=../similarity_search
NON_METRIC_SPACE_LIBRARY_INC=$(NON_METRIC_SPACE_LIBRARY)/include
NON_METRIC_SPACE_LIBRARY_LIB=$(NON_METRIC_SPACE_LIBRARY)/release
PYTHON_INC=/usr/include/python$(PYTHON_VERSION)
ADD_LIBS= -lrt -lgsl -lgslcblas -lboost_program_options -fopenmp

CXXFLAGS=-c -fpic -Wall -I$(PYTHON_INC) -I$(NON_METRIC_SPACE_LIBRARY_INC) -std=c++11 -Ofast -fno-strict-aliasing
LDFLAGS=-shared -lpython$(PYTHON_VERSION) -pthread 

SOURCES=nmslib.cc
OBJECTS=$(SOURCES:.cc=.o)
LIBS=$(NON_METRIC_SPACE_LIBRARY_LIB)/libNonMetricSpaceLib.a $(NON_METRIC_SPACE_LIBRARY_LIB)/liblshkit.a $(ADD_LIBS) 

FINALLIB=nmslib.so

all: $(SOURCES) $(FINALLIB)

install: all
	cp nmslib.so /usr/local/lib/python$(PYTHON_VERSION)/dist-packages


.PHONY: clean 

$(FINALLIB): $(OBJECTS) $(NON_METRIC_SPACE_LIBRARY_LIB)/libNonMetricSpaceLib.a
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(FINALLIB)

.cc.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(FINALLIB)


