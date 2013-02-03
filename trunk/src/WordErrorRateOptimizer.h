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

#ifndef WORDERRORRATEOPTIMIZER_H
#define WORDERRORRATEOPTIMIZER_H

#include <vector>
#include "optimize/Optimization.h"
#include "Types.h"
#include "NgramLM.h"
#include "Mask.h"
#include "Lattice.h"

using std::vector;

namespace mitlm {

////////////////////////////////////////////////////////////////////////////////

class WordErrorRateOptimizer {
protected:
    NgramLMBase &       _lm;
    size_t              _order;
    vector<Lattice *>   _lattices;
    size_t              _numCalls;
    double              _worstMargin;
    SharedPtr<Mask>     _mask;

    class ComputeMarginFunc {
        WordErrorRateOptimizer &_obj;
    public:
        ComputeMarginFunc(WordErrorRateOptimizer &obj) : _obj(obj) { }
        double operator()(const ParamVector &params)
        { _obj._numCalls++; return -_obj.ComputeMargin(params); }
    };

    class ComputeWERFunc {
        WordErrorRateOptimizer &_obj;
    public:
        ComputeWERFunc(WordErrorRateOptimizer &obj) : _obj(obj) { }
        double operator()(const ParamVector &params)
        { _obj._numCalls++; return _obj.ComputeWER(params); }
    };

public:
    WordErrorRateOptimizer(NgramLMBase &lm, size_t order=3)
        : _lm(lm), _order(order), _worstMargin(-100) { }
    ~WordErrorRateOptimizer();

    void   SetOrder(size_t order) { _order = order; }
    void   LoadLattices(ZFile &latticesFile);
    void   SaveLattices(ZFile &latticesFile);
    void   SaveTranscript(ZFile &transcriptFile);
    void   SaveUttConfidence(ZFile &confidenceFile);
    void   SaveWER(ZFile &werFile);
    double ComputeMargin(const ParamVector &params);
    double ComputeWER(const ParamVector &params);
    double ComputeOracleWER() const;
    double OptimizeMargin(ParamVector &params,
                          Optimization technique=PowellOptimization);
    double OptimizeWER(ParamVector &params,
                       Optimization technique=PowellOptimization);
};

}

#endif // WORDERRORRATEOPTIMIZER_H
