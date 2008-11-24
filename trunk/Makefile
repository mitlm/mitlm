INC      = -Isrc
CXXFLAGS = -g -Wall -fPIC -fmessage-length=0 $(INC)
LDFLAGS  = -lboost_program_options -lg2c
FFLAGS   = -g -fPIC -fmessage-length=0 

ifdef GPROF
	CXXFLAGS += -pg
	LDFLAGS  += -pg
endif

ifdef DEBUG
	CXXFLAGS += -O0 -fno-inline
else
	CXXFLAGS += -O3 -DNDEBUG -funroll-loops
	FFLAGS   += -O3 -DNDEBUG -funroll-loops
	LDFLAGS  += -O3 -funroll-loops
endif

UTIL_SOURCES = src/util/RefCounter.cpp src/util/Logger.cpp  
SOURCES      = $(UTIL_SOURCES) src/Vocab.cpp src/NgramVector.cpp \
               src/NgramModel.cpp src/NgramLM.cpp src/InterpolatedNgramLM.cpp \
               src/MaxLikelihoodSmoothing.cpp src/KneserNeySmoothing.cpp \
               src/PerplexityOptimizer.cpp src/WordErrorRateOptimizer.cpp \
               src/Lattice.cpp
UTIL_OBJECTS = $(UTIL_SOURCES:.cpp=.o)
OBJECTS      = $(SOURCES:.cpp=.o) src/optimize/lbfgsb.o src/optimize/lbfgs.o

# Core MITLM utilities
all: estimate-ngram interpolate-ngram evaluate-ngram

estimate-ngram: $(OBJECTS) src/estimate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/estimate-ngram.o -o $@ 

interpolate-ngram: $(OBJECTS) src/interpolate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/interpolate-ngram.o -o $@ 

evaluate-ngram: $(OBJECTS) src/evaluate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/evaluate-ngram.o -o $@ 

# Build scripts
clean:
	rm -f $(OBJECTS) src/*.o test/*.o mitlm.tgz 
	rm -f estimate-ngram interpolate-ngram evaluate-ngram

dist: clean
	cd ..; tar czvf mitlm.tgz --exclude=".*" mitlm/; cd mitlm; mv ../mitlm.tgz .  
