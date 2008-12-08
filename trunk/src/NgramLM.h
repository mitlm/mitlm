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

#ifndef NGRAMLM_H
#define NGRAMLM_H

#include <vector>
#include "util/SharedPtr.h"
#include "Types.h"
#include "Vocab.h"
#include "NgramModel.h"
#include "Smoothing.h"
#include "Mask.h"

using std::vector;

////////////////////////////////////////////////////////////////////////////////

class NgramLMBase {
    friend class PerplexityOptimizer;

protected:
    SharedPtr<NgramModel> _pModel;
    size_t                _order;
    vector<ProbVector>    _probVectors;
    vector<ProbVector>    _bowVectors;
    ParamVector           _defParams;

public:
    NgramLMBase(size_t order = 3);
    virtual ~NgramLMBase() { }
    void LoadVocab(const ZFile &vocabFile);
    void SaveVocab(const ZFile &vocabFile, bool asBinary=false) const;
    void SaveLM(const ZFile &lmFile, bool asBinary=false) const;
    void Serialize(FILE *outFile) const;
    void Deserialize(FILE *inFile);

    virtual void  SetOrder(size_t order);
    virtual Mask *GetMask(vector<BitVector> &probMaskVectors,
                          vector<BitVector> &bowMaskVectors) const;
    virtual bool  Estimate(const ParamVector &params, Mask *pMask=NULL);
    virtual void  SetModel(const SharedPtr<NgramModel> &m,
                           const VocabVector &vocabMap,
                           const vector<IndexVector> &ngramMap);

    size_t             order() const            { return _order; }
    size_t             sizes(size_t o) const    { return _pModel->sizes(o); }
    const Vocab &      vocab() const            { return _pModel->vocab(); }
    const NgramModel & model() const            { return *_pModel; }
    const VocabVector &words(size_t o) const    { return _pModel->words(o); }
    const IndexVector &hists(size_t o) const    { return _pModel->hists(o); }
    const IndexVector &backoffs(size_t o) const { return _pModel->backoffs(o); }
    const ProbVector  &probs(size_t o) const    { return _probVectors[o]; }
    const ProbVector  &bows(size_t o) const     { return _bowVectors[o]; }
    const ParamVector &defParams() const        { return _defParams; }
};

////////////////////////////////////////////////////////////////////////////////

class ArpaNgramLM : public NgramLMBase {
public:
    ArpaNgramLM(size_t order = 3) : NgramLMBase(order) { }
    void LoadLM(const ZFile &lmFile);
};

////////////////////////////////////////////////////////////////////////////////

class NgramLM : public NgramLMBase {
protected:
    vector<SharedPtr<Smoothing> >  _smoothings;
    vector<CountVector>            _countVectors;
    vector<FeatureVectors>         _featureList;
    IntVector                      _paramStarts;

public:
    NgramLM(size_t order = 3) : NgramLMBase(order), _countVectors(order + 1), 
                                _featureList(order + 1) { }
    void LoadCorpus(const ZFile &corpusFile, bool reset=false);
    void LoadCounts(const ZFile &countsFile, bool reset=false);
    void SaveCounts(const ZFile &countsFile, bool asBinary=false) const;
    void SaveEffCounts(const ZFile &countsFile, bool asBinary=false) const;
    void SetSmoothingAlgs(const vector<SharedPtr<Smoothing> > &smoothings);
    void SetWeighting(const vector<FeatureVectors> &featureList);

    virtual void  SetOrder(size_t order);
    virtual Mask *GetMask(vector<BitVector> &probMaskVectors,
                          vector<BitVector> &bowMaskVectors) const;
    virtual bool  Estimate(const ParamVector &params, Mask *pMask=NULL);
    virtual void  SetModel(const SharedPtr<NgramModel> &m,
                           const VocabVector &vocabMap,
                           const vector<IndexVector> &ngramMap);

    const CountVector    &counts(size_t o) const   { return _countVectors[o]; }
    const FeatureVectors &features(size_t o) const { return _featureList[o]; }
};

#endif // NGRAMLM_H
