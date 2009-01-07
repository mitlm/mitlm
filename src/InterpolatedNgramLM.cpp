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
    //    [LM1Params]...[LMlParams] [BiasParams] [Feat2Params]...[FeatlParams]
    _paramStarts.reset(_lms.size() + 1);
    VectorBuilder<Param> builder;
    for (size_t l = 0; l < _lms.size(); ++l) {
        _paramStarts[l] = builder.length();
        builder.append(_lms[l]->defParams());
    }
    _paramStarts[_lms.size()] = builder.length();
    builder.append(0, (_lms.size() - 1) * (_tieParamOrder ? 1 : order()));
    _defParams = builder;

    _featureList.resize(lms.size());
}

void
InterpolatedNgramLM::SetInterpolation(Interpolation interpolation,
                                      const vector<vector<FeatureVectors> > &featureList)
{
    // Make an attached clone of featureList.
    _featureList.resize(featureList.size());
    for (size_t l = 0; l < featureList.size(); ++l) {
        _featureList[l].resize(featureList[l].size());
        for (size_t f = 0; f < featureList[l].size(); ++f) {
            _featureList[l][f].resize(featureList[l][f].size());
            for (size_t o = 0; o < featureList[l][f].size(); ++o)
                _featureList[l][f][o].attach(featureList[l][f][o]);
        }
    }

    _interpolation = interpolation;
    switch (_interpolation) {
    case LinearInterpolation:
        for (size_t l = 0; l < _featureList.size(); ++l)
            assert(_featureList[l].size() == 0);
        break;
    case CountMerging:
        {
            assert(_featureList.size() == _lms.size());
            for (size_t l = 0; l < _featureList.size(); ++l)
                assert(_featureList[l].size() == 1);

            Range  r(_defParams.length());
            size_t numParams = r.length() +
                _lms.size() * (_tieParamOrder ? 1 : order());
            _paramDefaults.reset(numParams, 1);
            _paramDefaults[r] = _defParams;
            _paramMask.reset(numParams, false);
            _paramMask[r] = true;
        }
        break;
    case GeneralizedLinearInterpolation:
        {
            assert(_featureList.size() == _lms.size());

            Range  r(_defParams.length());
            size_t numParams = 0;
            if (_tieParamLM) {
                numParams = _featureList[0].size();
                for (size_t l = 1; l < _featureList.size(); ++l) {
                    if (_featureList[l].size() != numParams) {
                        throw std::runtime_error("TieParamLM requires "
                            "consistent number of features across LMs.");
                    }
                }
            } else {
                for (size_t l = 0; l < _featureList.size(); ++l)
                    numParams += _featureList[l].size();
            }
            if (!_tieParamOrder) numParams *= order();
            numParams += r.length();
            _paramDefaults.reset(numParams, 1);
            _paramDefaults[r] = _defParams;
            _paramMask.reset(numParams, true);
            _defParams.resize(numParams, 1);
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

    // Estimate component LMs.
    InterpolatedNgramLMMask *pLMMask = (InterpolatedNgramLMMask *)pMask;
    for (size_t l = 0; l < _lms.size(); ++l) {
        ParamVector lmParams(_paramDefaults[Range(_paramStarts[l],
                                                  _paramStarts[l+1])]);
        _lms[l]->Estimate(lmParams, pLMMask ? pLMMask->LMMasks[l].get() : NULL);
    }

    // Interpolate weighted probabilities and normalize backoff weights.
    ParamVector interpolationParams(_paramDefaults[
        Range(_paramStarts[_lms.size()], _paramDefaults.length())]);
    if (pLMMask != NULL) {
        _EstimateProbsMasked(interpolationParams, pLMMask);
        _EstimateBowsMasked(pLMMask);
    } else {
        _EstimateProbs(interpolationParams);
        _EstimateBows();
    }
    return true;
}

void
InterpolatedNgramLM::_EstimateProbs(const ParamVector &params) {
    const Param *pBiasParams = &params[0];
    const Param *pFeatParams = &params[(_lms.size() - 1) * 
                                       (_tieParamOrder ? 1 : order())];
    for (size_t o = 1; o <= _order; o++) {
        Range              r(sizes(o - 1));
        ProbVector         weights(_weights[r]);
        ProbVector         totWeights(_totWeights[r]);
        ProbVector &       probs(_probVectors[o]);
        const IndexVector &hists(this->hists(o));

        totWeights.set(0);
        probs.set(0);
        if (_tieParamOrder) {
            pBiasParams = &params[0];
            pFeatParams = &params[_lms.size() - 1];
        }
        const Param *pLMFeatParams = pFeatParams;
        for (size_t l = 0; l < _lms.size(); l++) {
            if (_tieParamLM)
                pFeatParams = pLMFeatParams;

            // Initialize weights with bias.
            weights.set((l == 0) ? 0 : *pBiasParams++);

            // Compute weights from log-linear combination of features.
            for (size_t f = 0; f < _featureList[l].size(); f++) {
                Param param = *pFeatParams++;
                if (param == 0) continue;
                // weights += _featureList[l][f][o-1] * param;
                for (size_t i = 0; i < _probVectors[o-1].length(); ++i)
                    weights[i] += _featureList[l][f][o-1][i] * param;
            }

            // Compute component weights and update total weights.
            // weights = exp(weights);
            // totWeights += weights;
            for (size_t i = 0; i < weights.length(); ++i) {
                weights[i] = exp(weights[i]);
                totWeights[i] += weights[i];
            }

            // Interpolate component LM probabilities.
            //probs += _lms[l]->probs(o) * weights[hists];
            const ProbVector &lmProbs(_lms[l]->probs(o));
            for (size_t i = 0; i < probs.length(); ++i)
                probs[i] += lmProbs[i] * weights[hists[i]];
        }
        assert(allTrue(totWeights != 0));
        assert(!anyTrue(isnan(probs)));
        assert(allTrue(hists >= 0));

        // Normalize probabilities.
        //probs /= totWeights[hists];
        for (size_t i = 0; i < probs.length(); ++i)
            probs[i] /= totWeights[hists[i]];
        assert(!anyTrue(isnan(probs)));
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
        assert(!anyTrue(isnan(bows)));
    }
}

void
InterpolatedNgramLM::_EstimateProbsMasked(const ParamVector &params,
                                          InterpolatedNgramLMMask *pMask) {
    assert(pMask != NULL);

    const Param *pBiasParams = &params[0];
    const Param *pFeatParams = &params[(_lms.size() - 1) * 
                                       (_tieParamOrder ? 1 : order())];
    for (size_t o = 1; o <= _order; o++) {
        Range              r(sizes(o - 1));
        ProbVector         weights(_weights[r]);
        ProbVector         totWeights(_totWeights[r]);
        ProbVector &       probs(_probVectors[o]);
        const IndexVector &hists(this->hists(o));

        totWeights.set(0);
        probs.set(0);
        if (_tieParamOrder) {
            pBiasParams = &params[0];
            pFeatParams = &params[_lms.size() - 1];
        }
        const Param *pLMFeatParams = pFeatParams;
        for (size_t l = 0; l < _lms.size(); ++l) {
            if (_tieParamLM)
                pFeatParams = pLMFeatParams;

            // Initialize weights with bias.
            weights.set((l == 0) ? 0 : *pBiasParams++);

            // Compute weights from log-linear combination of features.
            for (size_t f = 0; f < _featureList[l].size(); ++f) {
                Param param = *pFeatParams++;
                if (param == 0) continue;
                for (size_t i = 0; i < _probVectors[o-1].length(); ++i)
                    if (pMask->WeightMaskVectors[o-1][i])
                        weights[i] += _featureList[l][f][o-1][i] * param;
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
                //probs.mask(pMask->ProbMaskVectors[o]) +=
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
