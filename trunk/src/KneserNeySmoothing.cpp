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
#include "util/Logger.h"
#include "NgramLM.h"
#include "Mask.h"
#include "KneserNeySmoothing.h"

using std::min;

///////////////////////////////////////////////////////////////////////////////

void KneserNeySmoothing::Initialize(NgramLM *pLM, size_t order) {
    assert(order != 0);

    _pLM = pLM;
    _order = order;

    // Compute adjusted Kneser-Ney counts.
    if (_order < _pLM->order()) {
        // Compute left-branch counts.
        _effCounts.reset(_pLM->sizes(_order), 0);
        const CountVector &hoCounts(_pLM->counts(_order + 1));
        const IndexVector &hoBackoffs(_pLM->backoffs(_order + 1));
        // TODO: Need to implement operator[BitVector].
        //BinCount(hoBackoffs[hoCounts > 0], _effCounts);
        for (size_t i = 0; i < hoCounts.length(); i++)
            if (hoCounts[i] > 0)
                _effCounts[hoBackoffs[i]]++;

        // Use original counts for n-grams without left context.
        // Ex. N-grams starting with <s>.
        _effCounts.masked(_effCounts == 0) = _pLM->counts(_order);
    } else {
        // Use original counts for highest order.
        _effCounts.attach(_pLM->counts(_order));
    }

    if (pLM->features(order).size() > 0) {
        // If weighting features specified, cannot pre-compute invHistCounts.
        _invHistCounts.reset(_pLM->sizes(order - 1));
        _ngramWeights.reset(_pLM->sizes(order));
    } else {
        // Pre-compute inverse of sum of adjusted counts for each history.
        CountVector histCounts(_pLM->sizes(_order - 1), 0);
        BinWeight(_pLM->hists(_order), _effCounts, histCounts);
        _invHistCounts = CondExpr(histCounts == 0,
                                  0, (Param)1 / asDouble(histCounts));
    }

    // Estimate fixed Kneser-Ney discount factors from count stats.
    CountVector n(_discOrder + 2, 0);
    // BinCount(_effCounts[_effCounts < n.length()], n);
    BinClippedCount(_effCounts, n);
    double Y = (double)n[1] / (n[1] + 2*n[2]);
    //Range r(1, _discParams.length());
    //_discParams = CondExpr(n == 0, r, r - (r+1) * Y * n[r+1] / n[r]);
    //_discParams = min(r, max(0, _discParams));
    _discParams.resize(_discOrder + 1, 0);
    for (size_t i = 1; i < _discParams.length(); i++) {
        _discParams[i] = (n[i] == 0) ? i : (i - (i+1) * Y * n[i+1] / n[i]);
        if (_discParams[i] < 0) _discParams[i] = 0;
        if (_discParams[i] > i) _discParams[i] = i;
    }

    // Set default parameters.
    if (_tuneParams)
        _defParams = _discParams[Range(1, _discParams.length())];
    else
        _defParams.reset(0);

    // Set n-gram weighting parameters.
    _defParams.resize(_defParams.length() + _pLM->features(order).size(), 0);
}

void
KneserNeySmoothing::UpdateMask(NgramLMMask &lmMask) const {
    BitVector &        probMask(lmMask.ProbMaskVectors[_order]);
    BitVector &        boProbMask(lmMask.ProbMaskVectors[_order - 1]);
    BitVector &        boBowMask(lmMask.BowMaskVectors[_order - 1]);
    const IndexVector &histories(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));

    // Computing prob requires backoff prob and history bow.
    // boProbMask[backoffs] |= probMask;
    // boProbMask[histories] |= probMask;
    for (size_t i = 0; i < probMask.length(); i++) {
        if (probMask[i]) {
            if (!boProbMask[backoffs[i]])
                boProbMask[backoffs[i]] = true;
            if (!boBowMask[histories[i]])
                boBowMask[histories[i]] = true;
        }
    }

    // Compute discounts for any n-gram whose history is in bow mask.
    KneserNeySmoothingMask *pSmoothingMask = new KneserNeySmoothingMask();
    pSmoothingMask->DiscMask = boBowMask[histories];
    lmMask.SmoothingMasks[_order] = pSmoothingMask;
}

bool
KneserNeySmoothing::Estimate(const ParamVector &params,
                             const NgramLMMask *pMask,
                             ProbVector &probs,
                             ProbVector &bows) {

    // Unpack discount parameters.
    if (_tuneParams) {
        // Check of out-of-bounds discount parameters.
        for (size_t i = 0; i < _discOrder; i++)
            if (params[i] < 0 || params[i] > i+1) {
                Logger::Log(2, "Clipping\n");
                return false;
            }
        _discParams[Range(1, _discParams.length())] = params[Range(_discOrder)];
    }
    // Check of out-of-bounds n-gram weighting parameters.
    size_t numDiscParams = _tuneParams ? _discOrder : 0;
    for (size_t i = numDiscParams; i < params.length(); i++)
        if (fabs(params[i] > 100)) {
            Logger::Log(2, "Clipping\n");
            return false;
        }

    // Compute n-gram weights and inverse history counts, if necessary.
    size_t numFeatures = _pLM->features(_order).size();
    if (numFeatures > 0) {
        Range r(numDiscParams, numDiscParams + numFeatures);
        _ComputeWeights(ParamVector(params[r]));
        _invHistCounts.set(0);
        BinWeight(_pLM->hists(_order), _effCounts * _ngramWeights,
                  _invHistCounts);
        _invHistCounts = CondExpr(_invHistCounts == 0,
                                  0, (Param)1 / _invHistCounts);
    }

    // Estimate probs and bows using optimized methods.
    if (numFeatures > 0) {
        if (pMask != NULL)
            _EstimateWeightedMasked(pMask, probs, bows);
        else
            _EstimateWeighted(probs, bows);
    } else {
        if (pMask != NULL)
            _EstimateMasked(pMask, probs, bows);
        else
            _Estimate(probs, bows);
    }
    return true;
}

void
KneserNeySmoothing::_ComputeWeights(const ParamVector &featParams) {
    _ngramWeights.set(0);
    for (size_t f = 0; f < featParams.length(); ++f)
        if (featParams[f] != 0)
            _ngramWeights += _pLM->features(_order)[f] * featParams[f];
    _ngramWeights = exp(_ngramWeights);
}

void
KneserNeySmoothing::_Estimate(ProbVector &probs, ProbVector &bows) {
    const IndexVector &hists(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));
    const ProbVector & boProbs(_pLM->probs(_order - 1));

    // Compute discounts.
    ProbVector &discounts(probs);  // Reuse probs vector for discounts.
    discounts = _discParams[min(_effCounts, _discOrder)];

    // Compute backoff weights.
    bows.set(0);
    BinWeight(hists, discounts, bows);
    bows = CondExpr(_invHistCounts == 0, 1, bows * _invHistCounts);

    // Compute interpolated probabilities.
    if (_order == 1 && !_pLM->vocab().IsFixedVocab())
        probs = CondExpr(!_effCounts, 0,
                         (_effCounts - discounts) * _invHistCounts[hists]
                         + boProbs[backoffs] * bows[hists]);
    else
        probs = CondExpr(!_effCounts, 0,
                         (_effCounts - discounts) * _invHistCounts[hists])
                + boProbs[backoffs] * bows[hists];
}

void
KneserNeySmoothing::_EstimateMasked(const NgramLMMask *pMask,
                                    ProbVector &probs, ProbVector &bows) {
    const IndexVector &hists(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));
    const ProbVector & boProbs(_pLM->probs(_order - 1));

    // Compute discounts.
    ProbVector &discounts(probs);  // Reuse probs vector for discounts.
    const BitVector &discMask(((KneserNeySmoothingMask *)
                              pMask->SmoothingMasks[_order].get())->DiscMask);
    assert(discMask.length() == _effCounts.length());
//    discounts.masked(discMask) = _discParams[min(_effCounts, _discOrder)];
    for (size_t i = 0; i < _effCounts.length(); i++)
        if (discMask[i])
            discounts[i] = _discParams[min(_effCounts[i], (int)_discOrder)];

    // Compute backoff weights.
    const BitVector &bowMask(pMask->BowMaskVectors[_order - 1]);
    MaskedVectorClosure<ProbVector, BitVector> maskedBows(bows.masked(bowMask));
    maskedBows.set(0);
    BinWeight(hists, discounts, maskedBows);
//    maskedBows = CondExpr(_invHistCounts == 0, 1, bows * _invHistCounts);
    for (size_t i = 0; i < bows.length(); i++)
        if (bowMask[i]) {
            if (_invHistCounts[i] == 0)
                bows[i] = 1;
            else
                bows[i] *= _invHistCounts[i];
        }

    // Compute interpolated probabilities.
    const BitVector &probMask(pMask->ProbMaskVectors[_order]);
    if (_order == 1 && !_pLM->vocab().IsFixedVocab())
        probs.masked(probMask) =
            CondExpr(!_effCounts, 0,
                     (_effCounts - discounts) * _invHistCounts[hists]
                     + boProbs[backoffs] * bows[hists]);
    else
        probs.masked(probMask) =
            CondExpr(!_effCounts, 0,
                     (_effCounts - discounts) * _invHistCounts[hists])
            + boProbs[backoffs] * bows[hists];
}

void
KneserNeySmoothing::_EstimateWeighted(ProbVector &probs, ProbVector &bows) {
    const IndexVector &hists(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));
    const ProbVector & boProbs(_pLM->probs(_order - 1));

    // Compute discounts.
    ProbVector &discounts(probs);  // Reuse probs vector for discounts.
    discounts = _discParams[min(_effCounts, _discOrder)];

    // Compute backoff weights.
    bows.set(0);
    BinWeight(hists, _ngramWeights * discounts, bows);
    bows = CondExpr(_invHistCounts == 0, 1, bows * _invHistCounts);

    // Compute interpolated probabilities.
    if (_order == 1 && !_pLM->vocab().IsFixedVocab())
        probs = CondExpr(!_effCounts, 0,
                         _ngramWeights * (_effCounts - discounts)
                         * _invHistCounts[hists]
                         + boProbs[backoffs] * bows[hists]);
    else
        probs = CondExpr(!_effCounts, 0,
                         _ngramWeights * (_effCounts - discounts)
                         * _invHistCounts[hists])
            + boProbs[backoffs] * bows[hists];
}

void
KneserNeySmoothing::_EstimateWeightedMasked(const NgramLMMask *pMask,
                                            ProbVector &probs,
                                            ProbVector &bows) {
    const IndexVector &hists(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));
    const ProbVector & boProbs(_pLM->probs(_order - 1));

    // Compute discounts.
    ProbVector &discounts(probs);  // Reuse probs vector for discounts.
    const BitVector &discMask(((KneserNeySmoothingMask *)
                              pMask->SmoothingMasks[_order].get())->DiscMask);
    assert(discMask.length() == _effCounts.length());
//    discounts.masked(discMask) = _discParams[min(_effCounts, _discOrder)];
    for (size_t i = 0; i < _effCounts.length(); i++)
        if (discMask[i])
            discounts[i] = _discParams[min(_effCounts[i], (int)_discOrder)];

    // Compute backoff weights.
    const BitVector &bowMask(pMask->BowMaskVectors[_order - 1]);
    MaskedVectorClosure<ProbVector, BitVector> maskedBows(bows.masked(bowMask));
    maskedBows.set(0);
    BinWeight(hists, _ngramWeights * discounts, maskedBows);
//    maskedBows = CondExpr(_invHistCounts == 0, 1, bows * _invHistCounts);
    for (size_t i = 0; i < bows.length(); i++)
        if (bowMask[i]) {
            if (_invHistCounts[i] == 0)
                bows[i] = 1;
            else
                bows[i] *= _invHistCounts[i];
        }

    // Compute interpolated probabilities.
    const BitVector &probMask(pMask->ProbMaskVectors[_order]);
    if (_order == 1 && !_pLM->vocab().IsFixedVocab())
        probs.masked(probMask) =
            CondExpr(!_effCounts, 0,
                     _ngramWeights * (_effCounts - discounts)
                     * _invHistCounts[hists]
                     + boProbs[backoffs] * bows[hists]);
    else
        probs.masked(probMask) =
            CondExpr(!_effCounts, 0,
                     _ngramWeights * (_effCounts - discounts)
                     * _invHistCounts[hists])
            + boProbs[backoffs] * bows[hists];
}
