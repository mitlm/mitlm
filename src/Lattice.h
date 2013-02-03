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

#ifndef LATTICE_H
#define LATTICE_H

#include <vector>
#include "util/FastHash.h"
#include "util/ZFile.h"
#include "Types.h"
#include "NgramLM.h"

using std::vector;
using std::string;

////////////////////////////////////////////////////////////////////////////////

namespace mitlm {

const float INF = std::numeric_limits<float>::infinity();

inline float logAdd(float logX, float logY) {
   if (logY > logX)
       std::swap(logX, logY);
   float negDiff = logY - logX;
   if (negDiff < -20)
       return logX;
   return logX + std::log(1.0f + std::exp(negDiff));
}

class Lattice {
    friend class WordErrorRateOptimizer;

    struct ArcNgramIndex {
        ArcNgramIndex(uint a=0, uint o=0, NgramIndex i=0) :
            arcIndex(a), order(o), ngramIndex(i) { }
        uint       arcIndex : 28;
        uint       order    : 4;
        NgramIndex ngramIndex;
    };
    typedef DenseVector<ArcNgramIndex>  ArcNgramIndexVector;

    struct ArcScore {
        ArcScore(uint a, float s) : arc(a), score(s) { }
        uint  arc;
        float score;
    };
    typedef DenseVector<ArcScore>  ArcScoreVector;

    struct WordProb {
        WordProb(VocabIndex w, float p) : word(w), prob(p) { }
        VocabIndex word;
        float      prob;
    };

    const NgramLMBase & _lm;
    string              _tag;
    NodeIndex           _finalNode;
    NodeVector          _arcStarts;
    NodeVector          _arcEnds;
    VocabVector         _arcWords;
    FloatVector         _arcBaseWeights;
    FloatVector         _arcWeights;
    UIntVector          _nodeArcs;
    VocabVector         _ref;
    UIntVector          _oraclePath;
    int                 _oracleWER;
    ArcNgramIndexVector _arcProbs;
    ArcNgramIndexVector _arcBows;
    bool                _skipTags;

public:
    Lattice(const NgramLMBase &lm) : _lm(lm), _skipTags(true) { }
    void  SetTag(const char *tag) { _tag = tag; }
    void  LoadLattice(ZFile &latticeFile);
    void  SaveLattice(ZFile &latticeFile) const;
    void  UpdateWeights();
    void  SetReferenceText(const char *ref);
    float ComputeMargin() const;
    int   ComputeWER() const;
    void  GetBestPath(vector<VocabIndex> &bestPath) const;

    void  ComputeForwardScores(FloatVector &nodeScores) const;
    void  ComputeBackwardScores(FloatVector &nodeScores) const;
    void  ComputePosteriorProbs(const FloatVector &forwardScores,
                                const FloatVector &backwardScores,
                                FloatVector &arcProbs) const;
    void  ComputeForwardSteps(const FloatVector &forwardScores,
                             FloatVector &nodeSteps) const;
    void  ComputeBackwardSteps(const FloatVector &backwardScores,
                               FloatVector &nodeSteps) const;
    void  EstimateArcPosition(const FloatVector &forwardScores,
                              const FloatVector &backwardScores,
                              FloatVector &nodePositions) const;
    float BuildConfusionNetwork() const;

    void  Serialize(FILE *outFile) const;
    void  Deserialize(FILE *inFile);

    const char *       tag() const        { return _tag.c_str(); }
    const VocabVector &refWords() const   { return _ref; }
    const NodeVector  &arcStarts() const  { return _arcStarts; }
    const NodeVector  &arcEnds() const    { return _arcEnds; }
    const VocabVector &arcWords() const   { return _arcWords; }
    const FloatVector &arcWeights() const { return _arcWeights; }
    const UIntVector  &oraclePath() const { return _oraclePath; }
    int                oracleWER() const  { return _oracleWER; }
    NodeIndex          numNodes() const   { return _finalNode + 1; }

private:
    template <typename Compare>
    void  _Sort(size_t numTrans, const Compare &compare);
    void  _Reserve(size_t capacity);
    void  _ComputeArcNgramMapping();
    float _FindOraclePath();
    void  _ReverseViterbiSearch(ArcScoreVector &bestArcs) const;
    float _FindBestPath(const ArcScoreVector &bestArcs,
                        vector<VocabIndex> &bestPath) const;
    void  _FindNBestPaths(const ArcScoreVector &bestArcs,
                          size_t n, vector<float> &nbestScores) const;
    bool  _IsOracleBestPath(const ArcScoreVector &bestArcs) const;

};

}

#endif // LATTICE_H
