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
endif

UTIL_SOURCES = src/util/RefCounter.cpp src/util/Logger.cpp  
SOURCES      = $(UTIL_SOURCES) src/Vocab.cpp src/NgramVector.cpp \
               src/NgramModel.cpp src/NgramLM.cpp src/NgramPerplexity.cpp \
               src/MaxLikelihoodSmoothing.cpp src/KneserNeySmoothing.cpp \
               src/InterpolatedNgramLM.cpp
UTIL_OBJECTS = $(UTIL_SOURCES:.cpp=.o)
OBJECTS      = $(SOURCES:.cpp=.o) src/optimize/lbfgsb.o src/optimize/lbfgs.o

estimate-ngram: $(OBJECTS) src/estimate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/estimate-ngram.o -o $@ 

interpolate-ngram: $(OBJECTS) src/interpolate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/interpolate-ngram.o -o $@ 

evaluate-ngram: $(OBJECTS) src/evaluate-ngram.o
	$(CXX) $(LDFLAGS) $(OBJECTS) src/evaluate-ngram.o -o $@ 

TestMITLM: $(OBJECTS) test/TestMITLM.o
	$(CXX) $(LDFLAGS) $(OBJECTS) test/TestMITLM.o -lgtest -o $@ 

VectorTest: $(UTIL_OBJECTS) test/VectorTest.o
	$(CXX) $(LDFLAGS) $(UTIL_OBJECTS) test/VectorTest.o -lgtest -o $@

VectorTimeTest: $(UTIL_OBJECTS) test/VectorTimeTest.o
	$(CXX) $(LDFLAGS) $(UTIL_OBJECTS) test/VectorTimeTest.o -lgtest -o $@

time-estimate: estimate-ngram
	time ./estimate-ngram -d data/6.001-dev.txt -s ModKN \
	  data/switchboard.txt data/switchboard.lm.out 

time-interpolate: interpolate-ngram
	time interpolate-ngram -d data/6.001-dev.txt -L data/test.lm -i CM \
	  -P data/test.params \
	  data/switchboard.lm data/MIT-World.lm data/6.001-book.lm

time-interpolate-gli: interpolate-ngram
	time interpolate-ngram -d data/6.001-dev.txt -L data/test.lm -i GLI \
	  -f log:data/switchboard.counts -f log:data/MIT-World.counts \
	  -f log:data/6.001-book.counts -P data/test.params \
	  data/switchboard.lm data/MIT-World.lm data/6.001-book.lm

time-evaluate: evaluate-ngram
	time evaluate-ngram -e data/6.001-test.txt data/switchboard.lm

mitlm: estimate-ngram interpolate-ngram evaluate-ngram

test: TestMITLM VectorTest VectorTimeTest
	./TestMITLM
	./VectorTest
	./VectorTimeTest

time: time-estimate time-interpolate time-interpolate-gli time-evaluate

all: mitlm TestMITLM VectorTest VectorTimeTest

clean:
	rm -f $(OBJECTS) src/*.o test/*.o mitlm.tgz 
	rm -f VectorTest VectorTimeTest TestMITLM 
	rm -f estimate-ngram interpolate-ngram evaluate-ngram

dist: clean
	cd ..; tar czvf mitlm.tgz mitlm/; cd mitlm; mv ../mitlm.tgz .  
