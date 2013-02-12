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

#ifndef LBFGS_H
#define LBFGS_H

#include "../Types.h"

namespace mitlm {

////////////////////////////////////////////////////////////////////////////////

extern "C" {
    void mitlm_lbfgs_(int *n, int *m, double *x, double *f, double *g,
                int *diagco, double *diag, int *iprint,
                double *eps, double *xtol, double *w, int *iflag);
}

template <class Function>
double
MinimizeLBFGS(Function &func, DoubleVector &x, int &numIter, double step=1e-8,
              double eps=1e-5, double xtol=1e-16, int maxIter=0) {
    if (maxIter == 0) maxIter = 15000;

    int          n = x.length();
    int          m = 10;
    double       f;
    DoubleVector g(n);
    int          diagco = false;
    DoubleVector diag(n, 0);
    int          iprint[2] = {-1, 0};
    DoubleVector w(n*(2*m+1) + 2*m);
    int          iflag = 0;

    numIter = 0;
    while (true) {
        f = func(x);
        // Approximate gradient.
        for (int i = 0; i < n; ++i) {
            x[i] += step;
            g[i] = (func(x) - f) / step;
            x[i] -= step;
        }
        mitlm_lbfgs_(&n, &m, x.data(), &f, g.data(), &diagco, diag.data(), iprint,
               &eps, &xtol, w.data(), &iflag);
        if (iflag <= 0)
            break;
        if (++numIter > maxIter) break;
    }
    return f;
}

}

#endif // LBFGS_H
