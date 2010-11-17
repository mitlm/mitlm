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

#ifndef NGRAMMODEL_H
#define NGRAMMODEL_H

#include <vector>
#include "util/ZFile.h"
#include "Types.h"
#include "Vocab.h"
#include "NgramVector.h"

using std::vector;

////////////////////////////////////////////////////////////////////////////////
// NgramModel represents a full n-gram model, consisting of a NgramVector for
// each order of the model.  The model consists of a vocab mapping words to
// indices.  At the minimum, each n-gram is represented by its history index
// and target word index.  Additional fields, such as backoff index and counts,
// can be associated with each n-gram using vectors with aligned indices.
// Various methods are provides for efficiently loading/saving n-gram data.
//
class NgramModel {
protected:
    Vocab               _vocab;
    vector<NgramVector> _vectors;
    vector<IndexVector> _backoffVectors;

public:
    NgramModel(size_t order = 3);
    void   UseUnknown() { _vocab.UseUnknown(); }
    void   SetOrder(size_t order);
    void   LoadVocab(ZFile &vocabFile);
    void   SaveVocab(ZFile &vocabFile, bool asBinary=false) const;
    void   LoadCorpus(vector<CountVector> &countVectors,
                      ZFile &corpusFile, bool reset=false);
    void   LoadCounts(vector<CountVector> &countVectors,
                      ZFile &countsFile, bool reset=false);
    void   SaveCounts(const vector<CountVector> &countVectors,
                      ZFile &countsFile, bool includeZeroOrder=false) const;
    void   LoadLM(vector<ProbVector> &probVectors,
                  vector<ProbVector> &bowVectors,
                  ZFile &lmFile);
    void   SaveLM(const vector<ProbVector> &probVectors,
                  const vector<ProbVector> &bowVectors,
                  ZFile &lmFile) const;
    void   LoadEvalCorpus(vector<CountVector> &probCountVectors,
                          vector<CountVector> &bowCountVectors,
                          BitVector &vocabMask, ZFile &corpusFile,
                          size_t &outNumOOV, size_t &outNumWords) const;
    void   LoadFeatures(vector<DoubleVector> &featureVectors,
                        ZFile &featureFile, size_t maxOrder=0) const;
    void   LoadComputedFeatures(vector<DoubleVector> &featureVectors,
                                const char *featureFile,
                                size_t maxOrder=0) const;
    void   SaveFeatures(vector<DoubleVector> &featureVectors,
                        ZFile &featureFile) const;
    size_t GetNgramWords(size_t order, NgramIndex index, StrVector &wrds) const;
    void   ExtendModel(const NgramModel &m, VocabVector &vocabMap,
                       vector<IndexVector> &ngramMap);
    void   SortModel(VocabVector &vocabMap, vector<IndexVector> &ngramMap);
    void   Serialize(FILE *outFile) const;
    void   Deserialize(FILE *inFile);

    template <class T>
    static void ApplySort(const IndexVector &ngramMap, DenseVector<T> &data,
                          size_t length = 0, T defValue = T()) {
    assert(data.length() >= ngramMap.length());
    if (length == 0) length = ngramMap.length();
    DenseVector<T> sortedData(length, defValue);
    for (size_t i = 0; i < ngramMap.length(); ++i)
        sortedData[ngramMap[i]] = data[i];
    data.swap(sortedData);
    };

    size_t             size() const             { return _vectors.size(); }
    size_t             sizes(size_t o) const    { return _vectors[o].size(); }
    const Vocab &      vocab() const            { return _vocab; }
    const NgramVector &vectors(size_t o) const  { return _vectors[o]; }
    const VocabVector &words(size_t o) const    { return _vectors[o].words(); }
    const IndexVector &hists(size_t o) const    { return _vectors[o].hists(); }
    const IndexVector &backoffs(size_t o) const { return _backoffVectors[o];}

protected:
    NgramIndex _Find(const VocabIndex *words, size_t wordsLen) const;
    void       _ComputeBackoffs();
    void       _LoadFrequency(vector<DoubleVector> &freqVectors,
                              ZFile &corpusFile, size_t maxSize=0) const;
    void       _LoadEntropy(vector<DoubleVector> &entropyVectors,
                            ZFile &corpusFile, size_t maxSize=0) const;
    void       _LoadTopicProbs(vector<DoubleVector> &topicProbVectors,
                               ZFile &hmmldaFile, size_t maxSize,
                               bool onlyTargetWord=false) const;
    void       _LoadTopicProbs2(vector<DoubleVector> &topicProbVectors,
                                ZFile &hmmldaFile, size_t maxSize) const;
};

#endif // NGRAMMODEL_H
