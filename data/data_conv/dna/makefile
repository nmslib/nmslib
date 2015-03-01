LIB=-lboost_program_options

CXXFLAGS += 
# Enable C++11
CXXFLAGS += -std=c++11
# Debug flags
#CXXFLAGS += -g
CXXFLAGS += -O3

BIN=sample_dna_substr

all: $(BIN)

clean:
	rm -f *.o $(BIN)

# Note -pthread: this enables threads!!!
sample_dna_substr: sample_dna_substr.o makefile
	$(CXX) -o$@ sample_dna_substr.o $(LIB) 
