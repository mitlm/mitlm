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

#ifndef KNESERNEYSMOOTHING_H
#define KNESERNEYSMOOTHING_H

#include "Types.h"
#include "Smoothing.h"
#include "NgramLM.h"
#include "Mask.h"

///////////////////////////////////////////////////////////////////////////////

namespace mitlm {

class KneserNeySmoothing : public Smoothing {
protected:
    NgramLM     *_pLM;
    size_t       _order;
    size_t       _discOrder;
    bool         _tuneParams;
    ProbVector   _ngramWeights;
    ProbVector   _invHistCounts;
    ParamVector  _discParams;
    IntVector    _paramIndices;

public:
    KneserNeySmoothing(size_t discOrder=3, bool tuneParams=false)
        : _discOrder(discOrder), _tuneParams(tuneParams) { }
    virtual void Initialize(NgramLM *pLM, size_t order);
    virtual void UpdateMask(NgramLMMask &lmMask) const;
    virtual bool Estimate(const ParamVector &params, const NgramLMMask *pMask,
                          ProbVector &probs, ProbVector &bows);

protected:
    void _ComputeWeights(const ParamVector &featParams);
    void _Estimate(ProbVector &probs, ProbVector &bows);
    void _EstimateMasked(const NgramLMMask *pMask,
                         ProbVector &probs, ProbVector &bows);
    void _EstimateWeighted(ProbVector &probs, ProbVector &bows);
    void _EstimateWeightedMasked(const NgramLMMask *pMask,
                                 ProbVector &probs, ProbVector &bows);
};

}

#endif // KNESERNEYSMOOTHING_H
