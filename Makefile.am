## Process this file with automake to produce Makefile.in

SUBDIRS =

EXTRA_DIST = autogen.sh \
	debian/changelog \
	debian/control \
	debian/docs \
	debian/libmitlm-dev.install \
	debian/mitlm.install \
	debian/compat \
	debian/copyright \
	debian/info \
	debian/libmitlm.install  \
	debian/rules


INCLUDES = \
-Isrc

# Enable building files in subdirectories.
AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libmitlm.la

mitlmincdir = $(includedir)/mitlm
mitlmoptimizeincdir = $(includedir)/mitlm/optimize
mitlmutilincdir = $(includedir)/mitlm/util
mitlmvectorincdir = $(includedir)/mitlm/vector

mitlmoptimizeinc_HEADERS= \
	src/optimize/Optimization.h \
	src/optimize/LBFGS.h \
	src/optimize/Powell.h \
	src/optimize/LBFGSB.h

mitlmutilinc_HEADERS= \
	src/util/FastIO.h \
	src/util/RefCounter.h \
	src/util/CommandOptions.h \
	src/util/ZFile.h \
	src/util/Logger.h \
	src/util/SharedPtr.h \
	src/util/BitOps.h \
	src/util/FastHash.h

mitlmvectorinc_HEADERS= \
	src/vector/Range.h \
	src/vector/Operations.h \
	src/vector/VectorClosures.h \
	src/vector/Traits.h \
	src/vector/VectorBuilder.h \
	src/vector/VectorOps.h \
	src/vector/Scalar.h \
	src/vector/Vector.h \
	src/vector/DenseVector.h \
	src/vector/DenseVector.tcc

mitlminc_HEADERS= \
	src/MaxLikelihoodSmoothing.h \
	src/Smoothing.h \
	src/Vocab.h \
	src/Mask.h \
	src/KneserNeySmoothing.h \
	src/PerplexityOptimizer.h \
	src/NgramVector.h \
	src/Lattice.h \
	src/WordErrorRateOptimizer.h \
	src/InterpolatedNgramLM.h \
	src/NgramLM.h \
	src/NgramModel.h \
	src/Types.h



libmitlm_la_SOURCES = \
	src/util/CommandOptions.cpp \
	src/util/RefCounter.cpp \
	src/util/Logger.cpp \
	src/NgramLM.cpp \
	src/Vocab.cpp \
	src/PerplexityOptimizer.cpp \
	src/Lattice.cpp \
	src/Smoothing.cpp \
	src/NgramModel.cpp \
	src/NgramVector.cpp \
	src/MaxLikelihoodSmoothing.cpp \
	src/KneserNeySmoothing.cpp \
	src/InterpolatedNgramLM.cpp \
	src/optimize/lbfgs.f \
	src/optimize/lbfgsb.f \
	src/WordErrorRateOptimizer.cpp

libmitlm_la_LIBADD = $(FLIBS)


# Programs:

bin_PROGRAMS = evaluate-ngram estimate-ngram interpolate-ngram

evaluate_ngram_SOURCES = \
	src/evaluate-ngram.cpp

evaluate_ngram_LDADD = libmitlm.la $(FLIBS)
evaluate_ngram_CFLAGS = 

estimate_ngram_SOURCES = \
	src/estimate-ngram.cpp

estimate_ngram_LDADD = libmitlm.la $(FLIBS)
estimate_ngram_CFLAGS = 

interpolate_ngram_SOURCES = \
	src/interpolate-ngram.cpp

interpolate_ngram_LDADD = libmitlm.la $(FLIBS)
interpolate_ngram_CFLAGS = 