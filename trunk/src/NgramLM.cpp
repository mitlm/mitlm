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

#include <algorithm>
#include "util/FastIO.h"
#include "Types.h"
#include "NgramModel.h"
#include "NgramLM.h"
#include "MaxLikelihoodSmoothing.h"
#include "KneserNeySmoothing.h"

////////////////////////////////////////////////////////////////////////////////


NgramLMBase::NgramLMBase(size_t order)
    : _pModel(new NgramModel(order)), _order(order),
      _probVectors(order + 1), _bowVectors(order + 1) {
}

void
NgramLMBase::LoadVocab(ZFile &vocabFile) {
    _pModel->LoadVocab(vocabFile);
}

void
NgramLMBase::SaveVocab(ZFile &vocabFile, bool asBinary) const {
    _pModel->SaveVocab(vocabFile, asBinary);
}

void
NgramLMBase::SaveLM(ZFile &lmFile, bool asBinary) const {
    if (asBinary) {
        WriteUInt64(lmFile, MITLMv1);
        Serialize(lmFile);
    } else
        _pModel->SaveLM(_probVectors, _bowVectors, lmFile);
}

void
NgramLMBase::Serialize(FILE *outFile) const {
    WriteHeader(outFile, "NgramLM");
    _pModel->Serialize(outFile);
    for (size_t o = 0; o <= order(); ++o)
        WriteVector(outFile, _probVectors[o]);
    for (size_t o = 0; o < order(); ++o)
        WriteVector(outFile, _bowVectors[o]);
}

void
NgramLMBase::Deserialize(FILE *inFile) {
    VerifyHeader(inFile, "NgramLM");
    _pModel->Deserialize(inFile);
    SetOrder(_pModel->size() - 1);
    for (size_t o = 0; o <= order(); ++o)
        ReadVector(inFile, _probVectors[o]);
    for (size_t o = 0; o < order(); ++o)
        ReadVector(inFile, _bowVectors[o]);
}

void
NgramLMBase::SetOrder(size_t order) {
    _pModel->SetOrder(order);
    _order = order;
    _probVectors.resize(order + 1);
    _bowVectors.resize(order);
}

Mask *
NgramLMBase::GetMask(vector<BitVector> &probMaskVectors,
                     vector<BitVector> &bowMaskVectors) const {
    return NULL;
}

bool
NgramLMBase::Estimate(const ParamVector &params, Mask *pMask) {
    return true;
}

void
NgramLMBase::SetModel(const SharedPtr<NgramModel> &m,
                      const VocabVector &vocabMap,
                      const vector<IndexVector> &ngramMap) {
    for (size_t o = 1; o <= _order; ++o) {
        size_t len = m->sizes(o);
        NgramModel::ApplySort(ngramMap[o], _probVectors[o], len, (Prob)0);
        if (o < _order)
            NgramModel::ApplySort(ngramMap[o], _bowVectors[o], len, (Prob)1);
    }
    _pModel = m;

    // Fill in missing probabilities with backoff values.
    for (size_t o = 1; o <= _order; ++o) {
        IndexVector        hists(this->hists(o));
        const IndexVector &backoffs(this->backoffs(o));
        const ProbVector & boProbs(_probVectors[o-1]);
        const ProbVector & bows(_bowVectors[o-1]);
        ProbVector &       probs(_probVectors[o]);
        MaskAssign(probs == 0, boProbs[backoffs] * bows[hists], probs);
    }
}

////////////////////////////////////////////////////////////////////////////////

void
ArpaNgramLM::LoadLM(ZFile &lmFile) {
    if (ReadUInt64(lmFile) == MITLMv1) {
        Deserialize(lmFile);
    } else {
        lmFile.ReOpen();
        _pModel->LoadLM(_probVectors, _bowVectors, lmFile);
    }
}

////////////////////////////////////////////////////////////////////////////////

void
NgramLM::LoadCorpus(ZFile &corpusFile, bool reset) {
    _pModel->LoadCorpus(_countVectors, corpusFile, reset);
}

void
NgramLM::LoadCounts(ZFile &countsFile, bool reset) {
    if (ReadUInt64(countsFile) == MITLMv1) {
        if (!reset)
            throw std::runtime_error("Not implemented yet.");
        VerifyHeader(countsFile, "NgramCounts");
        _pModel->Deserialize(countsFile);
        SetOrder(_pModel->size() - 1);
        for (size_t o = 0; o <= order(); ++o)
            ReadVector(countsFile, _countVectors[o]);
    } else {
        countsFile.ReOpen();
        _pModel->LoadCounts(_countVectors, countsFile, reset);
    }
}

void
NgramLM::SaveCounts(ZFile &countsFile, bool asBinary) const {
    if (asBinary) {
        WriteUInt64(countsFile, MITLMv1);
        WriteHeader(countsFile, "NgramCounts");
        _pModel->Serialize(countsFile);
        for (size_t o = 0; o <= order(); ++o)
            WriteVector(countsFile, _countVectors[o]);
    } else {
        _pModel->SaveCounts(_countVectors, countsFile);
    }
}

void
NgramLM::SaveEffCounts(ZFile &countsFile, bool asBinary) const {
    vector<CountVector> effCountVectors(_order + 1);
    for (size_t o = 0; o < _order; ++o) {
        effCountVectors[o].reset(sizes(o), 0);
        const Smoothing *smoothing = (const Smoothing *)_smoothings[o + 1];
        effCountVectors[o].attach(smoothing->effCounts());
    }
    if (asBinary) {
        WriteUInt64(countsFile, MITLMv1);
        WriteHeader(countsFile, "NgramCounts");
        _pModel->Serialize(countsFile);
        for (size_t o = 0; o <= order(); ++o)
            WriteVector(countsFile, effCountVectors[o]);
    } else {
        _pModel->SaveCounts(effCountVectors, countsFile);
    }
}

void
NgramLM::SetSmoothingAlgs(const vector<SharedPtr<Smoothing> > &smoothings) {
    assert(smoothings.size() == _order + 1);
    _smoothings = smoothings;
    for (size_t o = 1; o <= _order; ++o) {
        assert(_smoothings[o]);
        _smoothings[o]->Initialize(this, o);
    }

    // Allocate and initialize variables.
    for (size_t o = 0; o < _order; ++o) {
        size_t len = _pModel->sizes(o);
        _probVectors[o].reset(len);
        _bowVectors[o].reset(len);
    }
    _probVectors[_order].reset(_pModel->sizes(_order));

    // Explicitly set count of <s> to 0.
    assert(words(1)[1] == Vocab::BeginOfSentence);
    _countVectors[1][1] = 0;

    // Compute 0th order probability (ignore 0 count unigrams).
    _probVectors[0][0] = Prob(1.0) / sum(_countVectors[1] > 0);

    // Compute default parameters.
    _paramStarts.reset(_order + 2);
    VectorBuilder<Param> builder;
    for (size_t o = 1; o <= _order; ++o) {
        _paramStarts[o] = builder.length();
        builder.append(_smoothings[o]->defParams());
    }
    _paramStarts[_order + 1] = builder.length();
    _defParams = builder;
}

void
NgramLM::SetWeighting(const vector<FeatureVectors> &featureList) {
    // NOTE: Remap featureList[f][o] to _featureList[o][f].
    if (featureList.size() > 0) _featureList.resize(featureList[0].size());
    for (size_t o = 0; o < _featureList.size(); ++o) {
        _featureList[o].resize(featureList.size());
        for (size_t f = 0; f < featureList.size(); ++f) {
            assert(featureList[f].size() == _featureList.size());
            _featureList[o][f].attach(featureList[f][o]);
        }
    }
}

void
NgramLM::SetOrder(size_t order) {
    NgramLMBase::SetOrder(order);
    _countVectors.resize(order + 1);
    _featureList.resize(order + 1);
}

Mask *
NgramLM::GetMask(vector<BitVector> &probMaskVectors,
                 vector<BitVector> &bowMaskVectors) const {
    // Copy prob and bow masks.
    NgramLMMask *pMask = new NgramLMMask();
    pMask->ProbMaskVectors = probMaskVectors;
    pMask->BowMaskVectors  = bowMaskVectors;

    // Let each smoothing algorithm update filter (in reverse).
    pMask->SmoothingMasks.resize(_order + 1);
    for (size_t o = _order; o > 0; o--)
        _smoothings[o]->UpdateMask(*pMask);
    return pMask;
}

bool
NgramLM::Estimate(const ParamVector &params, Mask *pMask) {
    NgramLMMask *pNgramLMMask = (NgramLMMask *)pMask;
    for (size_t o = 1; o <= _order; o++) {
        Range r(_paramStarts[o], _paramStarts[o+1]);
        if (!_smoothings[o]->Estimate(params[r], pNgramLMMask,
                                      _probVectors[o], _bowVectors[o-1]))
            return false;
    }
    return true;
}

void
NgramLM::SetModel(const SharedPtr<NgramModel> &m,
                  const VocabVector &vocabMap,
                  const vector<IndexVector> &ngramMap) {
    _pModel = m;
    for (size_t o = 1; o <= _order; ++o) {
        size_t len = m->sizes(o);
        NgramModel::ApplySort(ngramMap[o], _countVectors[o], len, 0);
        for (size_t f = 0; f < _featureList[o].size(); ++f)
            NgramModel::ApplySort(ngramMap[o], _featureList[o][f], len, 0.0);
    }
    SetSmoothingAlgs(_smoothings);
}
