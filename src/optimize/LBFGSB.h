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

#ifndef LBFGSB_H
#define LBFGSB_H

#include "../Types.h"

namespace mitlm {

////////////////////////////////////////////////////////////////////////////////

extern "C" {
    void mitlm_setulb(int *n, int *m, double *x, double *l, double *u, int *nbd,
                 double *f, double *g, double *factr, double *pgtol,
                 double *wa, int *iwa, char *task, int *iprint,
                 char *csave, int *lsave, int *isave, double *dsave);
}

template <class Function>
double
MinimizeLBFGSB(Function &func, DoubleVector &x, int &numIter, double step=1e-8,
               double factr=1e7, double pgtol=1e-5, int maxIter=0) {
    if (maxIter == 0) maxIter = 15000;

    int          n = x.length();
    int          m = 10;
    DoubleVector l(n);
    DoubleVector u(n);
    IntVector    nbd(n, 0);
    double       f;
    DoubleVector g(n);
    DoubleVector wa(2*m*n + 4*n + 12*m*m + 12*m);
    IntVector    iwa(3*n);
    char         task[60];
    int          iprint = -1;
    char         csave[60];
    IntVector    lsave(4);
    IntVector    isave(44);
    DoubleVector dsave(29);

    numIter = 0;
    memset(task, ' ', 60);
    strncpy(task, "START", 5);
    while (true) {
        mitlm_setulb(&n, &m, x.data(), l.data(), u.data(), nbd.data(),
                &f, g.data(), &factr, &pgtol, wa.data(), iwa.data(), &task[0],
                &iprint, &csave[0], lsave.data(), isave.data(), dsave.data());
        if (strncmp(task, "FG", 2) == 0) {
            f = func(x);
            // Approximate gradient.
            for (int i = 0; i < n; ++i) {
                x[i] += step;
                g[i] = (func(x) - f) / step;
                x[i] -= step;
            }
        } else if (strncmp(task, "NEW_X", 5) == 0) {
            if (++numIter >= maxIter)
                strcpy(task, "STOP: TOTAL NO. ITERATIONS EXCEEDS LIMIT");
        } else
            break;
    }
    return f;
}

}

#endif // LBFGSB_H
