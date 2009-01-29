INC      = -Isrc
CXXFLAGS = -g -Wall -fPIC -fmessage-length=0 $(INC)
LDFLAGS  = -L. -lg2c -lmitlm 
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

UTIL_SOURCES = src/util/RefCounter.cpp src/util/Logger.cpp src/util/CommandOptions.cpp
SOURCES      = $(UTIL_SOURCES) src/Vocab.cpp src/NgramVector.cpp \
               src/NgramModel.cpp src/NgramLM.cpp src/InterpolatedNgramLM.cpp \
               src/Smoothing.cpp src/MaxLikelihoodSmoothing.cpp src/KneserNeySmoothing.cpp \
               src/PerplexityOptimizer.cpp src/WordErrorRateOptimizer.cpp \
               src/Lattice.cpp 
UTIL_OBJECTS = $(UTIL_SOURCES:.cpp=.o)
OBJECTS      = $(SOURCES:.cpp=.o) src/optimize/lbfgsb.o src/optimize/lbfgs.o

# Core MITLM utilities
all: estimate-ngram interpolate-ngram evaluate-ngram

libmitlm.a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

estimate-ngram: libmitlm.a src/estimate-ngram.o
	$(CXX) src/estimate-ngram.o -o $@ $(LDFLAGS) 

interpolate-ngram: libmitlm.a src/interpolate-ngram.o
	$(CXX) src/interpolate-ngram.o -o $@ $(LDFLAGS) 

evaluate-ngram: libmitlm.a src/evaluate-ngram.o
	$(CXX) src/evaluate-ngram.o -o $@ $(LDFLAGS) 

# Build scripts
clean:
	rm -f $(OBJECTS) src/*.o test/*.o mitlm.tgz 
	rm -f estimate-ngram interpolate-ngram evaluate-ngram libmitlm.a

dist: clean
	cd ..; tar czvf mitlm.tgz --exclude=".*" mitlm/; cd mitlm; mv ../mitlm.tgz .  
