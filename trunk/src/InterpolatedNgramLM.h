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

#ifndef INTERPOLATEDNGRAMLM_H
#define INTERPOLATEDNGRAMLM_H

#include <vector>
#include "util/SharedPtr.h"
#include "Types.h"
#include "NgramModel.h"
#include "NgramLM.h"
#include "Mask.h"

using std::vector;

////////////////////////////////////////////////////////////////////////////////

enum Interpolation {
    LinearInterpolation = 0,
    CountMerging = 1,
    GeneralizedLinearInterpolation = 2,
    LI = LinearInterpolation,
    CM = CountMerging,
    GLI = GeneralizedLinearInterpolation
};

class InterpolatedNgramLM : public NgramLMBase {
protected:
    vector<SharedPtr<NgramLMBase> > _lms;
    vector<vector<DoubleVector> >   _featureList;
    Interpolation                   _interpolation;
    ProbVector                      _weights;
    ProbVector                      _totWeights;
    IntVector                       _paramStarts;
    ParamVector                     _paramDefaults;
    BitVector                       _paramMask;

public:
    InterpolatedNgramLM(size_t order = 3)
        : NgramLMBase(order), _interpolation(LI) { }
    void  LoadLMs(const vector<SharedPtr<NgramLMBase> > &lms);
    void  SetInterpolation(Interpolation interpolation,
                           const vector<FeatureVectors> &featureList);
    SharedPtr<NgramLMBase> &lms(int l) { return _lms[l]; }
    size_t                  numLMs()   { return _lms.size(); }

    virtual Mask *GetMask(vector<BitVector> &probMaskVectors,
                          vector<BitVector> &bowMaskVectors) const;
    virtual bool  Estimate(const ParamVector &params, Mask *pMask=NULL);

private:
    void _EstimateProbs(const ParamVector &params);
    void _EstimateBows();
    void _EstimateProbsMasked(const ParamVector &params,
                              InterpolatedNgramLMMask *pMask);
    void _EstimateBowsMasked(InterpolatedNgramLMMask *pMask);
};

#endif // INTERPOLATEDNGRAMLM_H
