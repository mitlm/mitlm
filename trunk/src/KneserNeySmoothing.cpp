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
        // BinCount(hoBackoffs[hoCounts > 0], _effCounts);
        const CountVector &hoCounts(_pLM->counts(_order + 1));
        const IndexVector &hoBackoffs(_pLM->backoffs(_order + 1));
        for (size_t i = 0; i < hoCounts.length(); i++)
            if (hoCounts[i] > 0)
                _effCounts[hoBackoffs[i]]++;

        // Use original counts for n-grams without left context.
        // Ex. Words starting with <s>.
        // _effCounts.masked(_effCounts == 0) = counts;
        MaskAssign(_effCounts == 0, _pLM->counts(_order), _effCounts);

        // Explicitly set count of <s> to 0.
        if (_order == 1) {
            assert(_pLM->words(_order)[1] == Vocab::BeginOfSentence);
            _effCounts[1] = 0;
        }
    } else {
        // Use original counts for highest order.
        _effCounts.attach(_pLM->counts(_order));
    }

    // Compute inverse of sum of adjusted counts for each history.
    CountVector histCounts(_pLM->sizes(_order - 1), 0);
    BinWeight(_pLM->hists(_order), _effCounts, histCounts);
    _invHistCounts = (Param)1 / asDouble(histCounts);

    // Estimate fixed Kneser-Ney discount factors from count stats.
    CountVector n(_discOrder + 2, 0);
    // BinCount(_effCounts[_effCounts < n.length()], n);
    BinClippedCount(_effCounts, n);
    double Y = (double)n[1] / (n[1] + 2*n[2]);
    // Range r(1, _discParams.length());
    // discParams = CondExpr(n == 0, r, r - (r+1) * Y * n[r+1] / n[r]);
    _discParams.resize(_discOrder + 1);
    for (size_t i = 1; i < _discParams.length(); i++) {
        _discParams[i] = (n[i] == 0) ? i : (i - (i+1) * Y * n[i+1] / n[i]);
        if (_discParams[i] < 0) _discParams[i] = 0;
        if (_discParams[i] > i) _discParams[i] = i;
    }

    // Set default parameters.
    if (_tuneParams)
        _defParams = _discParams[Range(1, _discParams.length())];
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
    // Check of out of bounds parameters.
    for (size_t i = 0; i < params.length(); i++)
        if (params[i] < 0 || params[i] > i+1) {
            Logger::Log(2, "Clipping\n");
            return false;
        }

    const IndexVector &histories(_pLM->hists(_order));
    const IndexVector &backoffs(_pLM->backoffs(_order));
    const ProbVector & boProbs(_pLM->probs(_order - 1));

    // Unpack discount parameters.
    if (_tuneParams)
        _discParams[Range(1, _discParams.length())] = params;

    // Compute discounts.
    ProbVector &discounts(probs);  // Reuse probs vector for discounts.
    // discounts.masked(discMask) = _discParams[min(_effCounts, _discOrder)];
    if (pMask != NULL) {
        BitVector &discMask(((KneserNeySmoothingMask *)
                              pMask->SmoothingMasks[_order].get())->DiscMask);
        assert(discMask.length() == _effCounts.length());
        for (size_t i = 0; i < _effCounts.length(); i++)
            if (discMask[i])
                discounts[i] = _discParams[min(_effCounts[i], (int)_discOrder)];
    } else {
        for (size_t i = 0; i < _effCounts.length(); i++)
            discounts[i] = _discParams[min(_effCounts[i], (int)_discOrder)];
    }

    // Compute backoff weights.
    bows.set(0);
    // BinWeight(histories, discounts, bows.masked(bowMask))
    // bows.masked(bowMask) =
    //     CondExpr(isnan(_invHistCount), 1.0, bows * _invHistCounts);
    if (pMask != NULL) {
        const BitVector &bowMask(pMask->BowMaskVectors[_order - 1]);
        for (size_t i = 0; i < _effCounts.length(); i++)
            if (bowMask[histories[i]])
                bows[histories[i]] += discounts[i];
        for (size_t i = 0; i < bows.length(); i++)
            if (bowMask[i]) {
                if (isnan(_invHistCounts[i]))
                    bows[i] = 1.0;
                else
                    bows[i] *= _invHistCounts[i];
            }
    } else {
        for (size_t i = 0; i < _effCounts.length(); i++)
            bows[histories[i]] += discounts[i];
        for (size_t i = 0; i < bows.length(); i++)
            if (isnan(_invHistCounts[i]))
                bows[i] = 1.0;
            else
                bows[i] *= _invHistCounts[i];
    }

    // Compute interpolated probabilities.
    // probs.masked(probMask) =
    //     CondExp(_effCounts == 0, 0,
    //             (_effCounts - discounts) * _invHistCounts[histories] +
    //             boProbs[backoffs] * bows[histories]);
    // probs.masked(probMask) =
    //     CondExp(_effCounts == 0, 0,
    //             (_effCounts - discounts) * _invHistCounts[histories]) +
    //     boProbs[backoffs] * bows[histories];
    if (pMask != NULL) {
         const BitVector &probMask(pMask->ProbMaskVectors[_order]);
         if (_order == 1) {
             for (size_t i = 0; i < probs.length(); i++) {
                 if (probMask[i]) {
                     // Force n-grams with 0 counts (<s>) to 0 prob.
                     if (_effCounts[i] == 0)
                         probs[i] = 0.0;
                     else {
                         double discCount = _effCounts[i] - discounts[i];
                         probs[i] = discCount * _invHistCounts[histories[i]]
                             + boProbs[backoffs[i]] * bows[histories[i]];
                     }
                 }
             }
         } else {
             for (size_t i = 0; i < probs.length(); i++) {
                 if (probMask[i]) {
                     // Note that discounts and probs share same buffer.
                     double p = boProbs[backoffs[i]] * bows[histories[i]];
                     if (_effCounts[i] > 0) {
                         double discCount = _effCounts[i] - discounts[i];
                         p += discCount * _invHistCounts[histories[i]];
                     }
                     probs[i] = p;
                 }
             }
         }
    } else {
        if (_order == 1) {
            for (size_t i = 0; i < probs.length(); i++) {
                // Force n-grams with 0 counts (<s>) to 0 prob.
                if (_effCounts[i] == 0)
                    probs[i] = 0.0;
                else {
                    double discCount = _effCounts[i] - discounts[i];
                    probs[i] = discCount * _invHistCounts[histories[i]]
                        + boProbs[backoffs[i]] * bows[histories[i]];
                }
            }
        } else {
            for (size_t i = 0; i < probs.length(); i++) {
                // Note that discounts and probs share same buffer.
                double p = boProbs[backoffs[i]] * bows[histories[i]];
                if (_effCounts[i] > 0) {
                    double discCount = _effCounts[i] - discounts[i];
                    p += discCount * _invHistCounts[histories[i]];
                }
                probs[i] = p;
            }
        }
    }
    return true;
}


