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

#include "Types.h"
#include "NgramModel.h"
#include "NgramLM.h"
#include "InterpolatedNgramLM.h"

////////////////////////////////////////////////////////////////////////////////

void
InterpolatedNgramLM::LoadLMs(const vector<SharedPtr<NgramLMBase> > &lms) {
    _lms = lms;

    // Build merged NgramModel and sort.
    vector<VocabVector>          vocabMaps(_lms.size());
    vector<vector<IndexVector> > ngramMaps(_lms.size());
    for (size_t l = 0; l < _lms.size(); l++)
        _pModel->ExtendModel(_lms[l]->model(), vocabMaps[l], ngramMaps[l]);

    // Sort n-grams.
    VocabVector         vocabSortMap;
    vector<IndexVector> ngramSortMap;
    _pModel->SortModel(vocabSortMap, ngramSortMap);

    // Update component LMs to use sorted merged NgramModel.
    for (size_t l = 0; l < _lms.size(); l++) {
        VocabVector         vocabMap = vocabSortMap[vocabMaps[l]];
        vector<IndexVector> ngramMap(ngramMaps[l].size());
        for (size_t o = 1; o < ngramMaps[l].size(); o++)
            ngramMap[o] = ngramSortMap[o][ngramMaps[l][o]];
        _lms[l]->SetModel(_pModel, vocabMap, ngramMap);
    }

    // Allocate and initialize variables.
    size_t maxLen = 0;
    for (size_t o = 0; o <= _order; ++o) {
        size_t len = _pModel->sizes(o);
        _probVectors[o].reset(len);
        if (o < _order) _bowVectors[o].reset(len);
        if (len > maxLen) maxLen = len;
    }
    _weights.reset(maxLen);
    _totWeights.reset(maxLen);

    // Compute 0th order probability.
    _probVectors[0][0] = 0;

    // Compute default parameters.
    //    [BiasParams] [LM1Params]...[LMlParams] [Feat2Params]...[FeatlParams]
    _paramStarts.reset(_lms.size() + 1);
    VectorBuilder<Param> builder;
    builder.append(0, _lms.size() - 1);
    for (size_t l = 0; l < _lms.size(); ++l) {
        _paramStarts[l] = builder.length();
        builder.append(_lms[l]->defParams());
    }
    _paramStarts[_lms.size()] = builder.length();
    _defParams = builder;
}

void
InterpolatedNgramLM::SetInterpolation(Interpolation interpolation,
                                      const vector<FeatureVectors> &featureList)
{
    _featureList.resize(featureList.size());
    for (size_t f = 0; f < featureList.size(); ++f) {
        _featureList[f].resize(featureList[f].size());
        for (size_t o = 0; o < featureList[f].size(); ++o)
            _featureList[f][o].attach(featureList[f][o]);
    }

    _interpolation = interpolation;
    switch (_interpolation) {
    case LinearInterpolation:
        assert(_featureList.size() == 0);
        break;
    case CountMerging:
        {
            assert(_featureList.size() == _lms.size());
            Range  r(_defParams.length());
            size_t numParams = r.length() + _lms.size()*_lms.size();
            _paramDefaults.reset(numParams, 0);
            _paramDefaults[r] = _defParams;
            _paramMask.reset(numParams, 0);
            _paramMask[r] = true;
            for (size_t l = 0; l < _lms.size(); ++l)
                _paramDefaults[r.length() + l * _lms.size() + l] = 1;
        }
        break;
    case GeneralizedLinearInterpolation:
        {
            Range  r(_defParams.length());
            size_t numParams = r.length() + _lms.size() * _featureList.size();
            _paramDefaults.reset(numParams, 0);
            _paramDefaults[r] = _defParams;
            _paramMask.reset(numParams, true);
            _defParams.resize(numParams, 0);
        }
        break;
    }
}

Mask *
InterpolatedNgramLM::GetMask(vector<BitVector> &probMaskVectors,
                               vector<BitVector> &bowMaskVectors) const {
    // Extend prob and bow masks.
    InterpolatedNgramLMMask *pMask = new InterpolatedNgramLMMask();
    pMask->ProbMaskVectors.resize(_order + 1);
    pMask->BowMaskVectors.resize(_order);
    pMask->WeightMaskVectors.resize(_order);
    pMask->ProbMaskVectors[0] = probMaskVectors[0];
    for (size_t o = 1; o <= _order; o++) {
        BitVector &probMasks(pMask->ProbMaskVectors[o]);
        BitVector &boProbMasks(pMask->ProbMaskVectors[o - 1]);
        BitVector &bowMasks(bowMaskVectors[o-1]);
        IndexVector hists(this->hists(o));
        const IndexVector &backoffs(this->backoffs(o));

        // probMasks = probMaskVectors[o] | bowMasks[hists];
        // boProbMasks[backoffs] |= bowMasks[hists];
        probMasks = probMaskVectors[o];
        for (size_t i = 0; i < probMasks.length(); ++i) {
            if (bowMasks[hists[i]]) {
                probMasks[i] = 1;
                boProbMasks[backoffs[i]] = 1;
            }
        }
    }
    for (size_t o = 0; o < _order; ++o) {
        pMask->BowMaskVectors[o] = bowMaskVectors[o];

        BitVector &weightMasks(pMask->WeightMaskVectors[o]);
        BitVector &hoProbMasks(pMask->ProbMaskVectors[o + 1]);
        IndexVector hoHists(this->hists(o + 1));

        // weightMasks[hoHists] |= hoProbMasks;
        weightMasks.resize(sizes(o));
        for (size_t i = 0; i < hoProbMasks.length(); ++i)
            if (hoProbMasks[i])
                weightMasks[hoHists[i]] = 1;
    }

    // Compute filter for each component LM.
    pMask->LMMasks.resize(_lms.size());
    for (size_t l = 0; l < _lms.size(); ++l) {
        pMask->LMMasks[l] = _lms[l]->GetMask(
            pMask->ProbMaskVectors, pMask->BowMaskVectors);
    }
    return pMask;
}

bool
InterpolatedNgramLM::Estimate(const ParamVector &params, Mask *pMask) {
    // Map parameters.
    if (_paramMask.length()) {
        const Param *p = params.begin();
        for (size_t i = 0; i < _paramMask.length(); ++i) {
            if (_paramMask[i])
                _paramDefaults[i] = *p++;
        }
    } else {
        _paramDefaults = params;
    }

    // TODO: Estimate component LMs.

    // Interpolate weighted probabilities and normalize backoff weights.
    if (pMask != NULL) {
        InterpolatedNgramLMMask *pLMMask = (InterpolatedNgramLMMask *)pMask;
        _EstimateProbsMasked(_paramDefaults, pLMMask);
        _EstimateBowsMasked(pLMMask);
    } else {
        _EstimateProbs(_paramDefaults);
        _EstimateBows();
    }
    return true;
}

void
InterpolatedNgramLM::_EstimateProbs(const ParamVector &params) {
    size_t numFeatures = _featureList.size();
    for (size_t o = 1; o <= _order; o++) {
        Range              r(sizes(o - 1));
        ProbVector         weights(_weights[r]);
        ProbVector         totWeights(_totWeights[r]);
        ProbVector &       probs(_probVectors[o]);
        const IndexVector &hists(this->hists(o));

        totWeights.set(0);
        probs.set(0);
        for (size_t l = 0; l < _lms.size(); l++) {
            // Initialize weights with bias.
            weights.set((l == 0) ? 0 : params[l - 1]);

            // Compute weights from log-linear combination of features.
            size_t startIndex = _paramStarts[_paramStarts.length() - 1] +
                                l * numFeatures;
            assert(startIndex + numFeatures <= params.length());
            ParamVector featParams = params[Range(startIndex,
                                                  startIndex + numFeatures)];
            for (size_t f = 0; f < numFeatures; f++) {
                if (featParams[f] == 0) continue;
                for (size_t i = 0; i < _probVectors[o-1].length(); ++i)
                    weights[i] += _featureList[f][o-1][i] * featParams[f];
            }

            // Compute component weights and update total weights.
            for (size_t i = 0; i < weights.length(); ++i) {
                weights[i] = exp(weights[i]);
                totWeights[i] += weights[i];
            }

            // Interpolate component LM probabilities.
            const ProbVector &lmProbs(_lms[l]->probs(o));
            for (size_t i = 0; i < probs.length(); ++i)
                probs[i] += lmProbs[i] * weights[hists[i]];
        }
        // Normalize probabilities.
        for (size_t i = 0; i < probs.length(); ++i)
            probs[i] /= totWeights[hists[i]];
    }
}

void
InterpolatedNgramLM::_EstimateBows() {
    for (size_t o = 1; o <= _order; o++) {
        ProbVector &       bows(_bowVectors[o - 1]);
        const ProbVector & probs(this->probs(o));
        const ProbVector & boProbs(this->probs(o - 1));
        const IndexVector &hists(this->hists(o));
        const IndexVector &backoffs(this->backoffs(o));

        Range      r(sizes(o - 1));
        ProbVector numerator(_weights[r]);       // Reuse buffers.
        ProbVector denominator(_totWeights[r]);  // Reuse buffers.
        numerator.set(0);
        denominator.set(0);

        for (size_t i = 0; i < probs.length(); ++i) {
            numerator[hists[i]] += probs[i];
            denominator[hists[i]] += boProbs[backoffs[i]];
        }
        for (size_t i = 0; i < bows.length(); ++i)
            bows[i] = (1 - numerator[i]) / (1 - denominator[i]);
    }
}

void
InterpolatedNgramLM::_EstimateProbsMasked(const ParamVector &params,
                                    InterpolatedNgramLMMask *pMask) {
    size_t numFeatures = _featureList.size();
    for (size_t o = 1; o <= _order; o++) {
        Range              r(sizes(o - 1));
        ProbVector         weights(_weights[r]);
        ProbVector         totWeights(_totWeights[r]);
        ProbVector &       probs(_probVectors[o]);
        const IndexVector &hists(this->hists(o));

        totWeights.set(0);
        probs.set(0);
        for (size_t l = 0; l < _lms.size(); l++) {
            // Initialize weights with bias.
            weights.set((l == 0) ? 0 : params[l - 1]);

            // Compute weights from log-linear combination of features.
            size_t startIndex = _paramStarts[_paramStarts.length() - 1] +
                                l * numFeatures;
            assert(startIndex + numFeatures <= params.length());
            ParamVector featParams = params[Range(startIndex,
                                                  startIndex + numFeatures)];
            for (size_t f = 0; f < numFeatures; f++) {
                if (featParams[f] == 0) continue;
                if (pMask) {
                    for (size_t i = 0; i < _probVectors[o-1].length(); ++i)
                        if (pMask->WeightMaskVectors[o-1][i])
                            weights[i] += _featureList[f][o-1][i]*featParams[f];
                } else {
                    for (size_t i = 0; i < _probVectors[o-1].length(); ++i)
                        weights[i] += _featureList[f][o-1][i] * featParams[f];
                }
            }

            // Compute component weights and update total weights.
            //weights.mask(pMask->WeightMaskVectors[o - 1]) = exp(weights);
            //totWeights.mask(pMask->WeightMaskVectors[o - 1]) += weights;
            for (size_t i = 0; i < weights.length(); ++i) {
                if (pMask->WeightMaskVectors[o-1][i]) {
                    weights[i] = exp(weights[i]);
                    totWeights[i] += weights[i];
                }
            }

            // Interpolate component LM probabilities.
            const ProbVector &lmProbs(_lms[l]->probs(o));
            for (size_t i = 0; i < probs.length(); ++i)
                //probs.mask(pMask->ProbMaskVectors[o]) += \
                //    lmProbs * weights[hists];
                if (pMask->ProbMaskVectors[o][i])
                    probs[i] += lmProbs[i] * weights[hists[i]];
        }
        // Normalize probabilities.
        for (size_t i = 0; i < probs.length(); ++i)
            if (pMask->ProbMaskVectors[o][i])
                probs[i] /= totWeights[hists[i]];
    }
}

void
InterpolatedNgramLM::_EstimateBowsMasked(InterpolatedNgramLMMask *pMask) {
    for (size_t o = 1; o <= _order; o++) {
        ProbVector &       bows(_bowVectors[o - 1]);
        const ProbVector & probs(this->probs(o));
        const ProbVector & boProbs(this->probs(o - 1));
        const IndexVector &hists(this->hists(o));
        const IndexVector &backoffs(this->backoffs(o));

        Range      r(sizes(o - 1));
        ProbVector numerator(_weights[r]);       // Reuse buffers.
        ProbVector denominator(_totWeights[r]);  // Reuse buffers.
        numerator.set(0);
        denominator.set(0);

        // BitVector m = pMask->BowMaskVectors[o-1][hists];
        // numerator[hists].masked(m) += probs;
        // denominator[hists].masked(m) += boProbs[backoffs];
        for (size_t i = 0; i < probs.length(); ++i) {
            if (pMask->BowMaskVectors[o-1][hists[i]]) {
                numerator[hists[i]] += probs[i];
                denominator[hists[i]] += boProbs[backoffs[i]];
            }
        }
        //bows.masked(pMask->BowMaskVectors[o-1]) = (1 - numerator) /
        //                                          (1 - denominator);
        for (size_t i = 0; i < bows.length(); ++i)
            if (pMask->BowMaskVectors[o-1][i])
                bows[i] = (1 - numerator[i]) / (1 - denominator[i]);
    }
}
