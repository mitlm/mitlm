////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// All rights reserved.                                                   //
//                                                                        //
// Redistribution and use in source and binary forms, with or without     //
// modification, are permitted provided that the following conditions are //
// met:                                                                   //
//                                                                        //
//     * Redistributions of source code must retain the above copyright   //
//       notice, this list of conditions and the following disclaimer.    //
//                                                                        //
//     * Redistributions in binary form must reproduce the above          //
//       copyright notice, this list of conditions and the following      //
//       disclaimer in the documentation and/or other materials provided  //
//       with the distribution.                                           //
//                                                                        //
//     * Neither the name of the Massachusetts Institute of Technology    //
//       nor the names of its contributors may be used to endorse or      //
//       promote products derived from this software without specific     //
//       prior written permission.                                        //
//                                                                        //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    //
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      //
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  //
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   //
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  //
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       //
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  //
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    //
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  //
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   //
////////////////////////////////////////////////////////////////////////////

#include <ctime>
#include "util/Logger.h"
#include "NgramPerplexity.h"

////////////////////////////////////////////////////////////////////////////////

void
NgramPerplexity::LoadCorpus(const ZFile &corpusFile) {
    //const CountVector &counts(_lm.counts(1));
    //BitVector vocabMask = (counts > 0);
    BitVector vocabMask(_lm.sizes(1), 1);
    _lm._pModel->LoadEvalCorpus(_probCountVectors, _bowCountVectors,
                                vocabMask, corpusFile, _numOOV, _numWords);

    vector<BitVector> probMaskVectors(_order + 1);
    vector<BitVector> bowMaskVectors(_order + 1);
    for (size_t o = 0; o <= _order; o++)
        probMaskVectors[o] = (_probCountVectors[o] > 0);
    for (size_t o = 0; o < _order; o++)
        bowMaskVectors[o] = (_bowCountVectors[o] > 0);
    _mask = _lm.GetMask(probMaskVectors, bowMaskVectors);
}

double
NgramPerplexity::ComputeEntropy(const ParamVector &params) {
    // Estimate model.
    if (!_lm.Estimate(params, _mask))
        return 7;  // Out of bounds.  Corresponds to perplexity = 1100.

    // Compute total log probability and num zero probs.
    _totLogProb = 0.0;
    _numZeroProbs = 0;
    for (size_t o = 0; o <= _order; o++) {
        // assert(alltrue(counts == 0 || probs > 0));
        // _totLogProb += dot(log(probs), counts, counts > 0);
        // _totLogProb += sum((log(probs) * counts)[counts > 0]);
        const CountVector &counts(_probCountVectors[o]);
        const ProbVector & probs(_lm.probs(o));
        for (size_t i = 0; i < counts.length(); i++) {
            if (counts[i] > 0) {
                if (probs[i] == 0)
                    _numZeroProbs++;
                else
                    _totLogProb += log(probs[i]) * counts[i];
            }
        }
    }
    for (size_t o = 0; o < _order; o++) {
        // assert(allTrue(counts == 0 || bows > 0));
        // _totLogProb += dot(log(bows), counts, counts > 0);
        const CountVector &counts(_bowCountVectors[o]);
        const ProbVector & bows(_lm.bows(o));
        for (size_t i = 0; i < counts.length(); i++) {
            if (counts[i] > 0) {
                assert(bows[i] != 0);
                if (bows[i] == 0)
                    Logger::Warn(1, "Invalid BOW %lu %lu %i\n", o,i,counts[i]);
                _totLogProb += log(bows[i]) * counts[i];
            }
        }
    }

    double entropy = -_totLogProb / (_numWords - _numZeroProbs);
    if (Logger::GetVerbosity() > 2)
        std::cout << exp(entropy) << "\t" << params << std::endl;
    else
        Logger::Log(2, "%f\n", exp(entropy));
    return entropy;
}

double
NgramPerplexity::Optimize(ParamVector &params, Optimization technique) {
    _numCalls = 0;
    ComputeEntropyFunc func(*this);
    int     numIter;
    double  minEntropy;
    clock_t startTime = clock();
    switch (technique) {
    case PowellOptimization:
        minEntropy = MinimizePowell(func, params, numIter);
        break;
    case LBFGSOptimization:
        minEntropy = MinimizeLBFGS(func, params, numIter);
        break;
    case LBFGSBOptimization:
        minEntropy = MinimizeLBFGSB(func, params, numIter);
        break;
    default:
        throw "Unsupported optimization technique.";
    }
    clock_t endTime = clock();

    Logger::Log(1, "Iterations    = %i\n", numIter);
    Logger::Log(1, "Elapsed Time  = %f\n",
                float(endTime - startTime) / CLOCKS_PER_SEC);
    Logger::Log(1, "Perplexity    = %f\n", exp(minEntropy));
    Logger::Log(1, "Num OOVs      = %lu\n", _numOOV);
    Logger::Log(1, "Num ZeroProbs = %lu\n", _numZeroProbs);
    Logger::Log(1, "Func Evals    = %lu\n", _numCalls);
    Logger::Log(1, "OptParams     = [ ");
    for (size_t i = 0; i < params.length(); i++)
        Logger::Log(1, "%f ", params[i]);
    Logger::Log(1, "]\n");
    return minEntropy;
}
