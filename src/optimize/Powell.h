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

#ifndef POWELL_H
#define POWELL_H

#include <vector>
#include "../Types.h"

////////////////////////////////////////////////////////////////////////////////
// Implementation of Powell's Method modified from Numerical Recipes in C.

template<class Function>
class Function1D {
    Function &         _func;
    const DoubleVector &_x;
    const DoubleVector &_dir;
    DoubleVector        _p;

public:
    Function1D(Function &func, const DoubleVector &x, const DoubleVector &dir)
        : _func(func), _x(x), _dir(dir), _p(x.length()) { }
    double operator()(double alpha) {
        for (size_t i = 0; i < _x.length(); i++)
            _p[i] = _x[i] + alpha * _dir[i];
        return _func(_p);
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class Function>
double
MinimizePowell(Function &func, DoubleVector &x, int &numIter,
               double xTol=1e-4, double fTol=1e-4, int maxIter=0) {
    int N = x.length();
    if (maxIter == 0) maxIter = N * 1000;

    // Initialize dir set to N unit vectors.
    DoubleVector              overallDir(N);
    std::vector<DoubleVector> dirSet(N);
    for (int i = 0; i < N; i++) {
        dirSet[i].resize(N, 0);
        dirSet[i][i] = 1.0;
    }

    DoubleVector  xHyp(N);
    DoubleVector  xStart(N);
    double        f = func(x);

    for (numIter = 0; numIter < maxIter; numIter++) {
        int    argMaxDelta = 0;
        double maxDelta    = 0.0;
        double fStart      = f;
        for (int i = 0; i < N; i++)
            xStart[i] = x[i];
        for (int i = 0; i < N; i++) {
            double fPrev = f;
            f = LineSearch(func, x, dirSet[i], xTol*100);
            // Remember direction with largest decrease..
            if ((fPrev - f) > maxDelta) {
                maxDelta = fPrev - f;
                argMaxDelta = i;
            }
        }

        // Check for termination conditions.
        if (2 * (fStart-f) <= fTol * (fabs(fStart)+fabs(f)) + 1e-20)
            break;

        // Minimize in overall direction traversed during this iteration.
        for (int i = 0; i < N; i++) {
            overallDir[i] = x[i] - xStart[i];
            xHyp[i] = x[i] + overallDir[i];
            // NOTE: NumericalRecipes update xStart here instead of at beginning
            // of each iteration.  On the Rosenbrock function, this is slower
            // than updating at the beginning of each iteration.
        }
        double fHyp = func(xHyp);
        if (fHyp < fStart) {
            double t1 = fStart-f-maxDelta;
            double t2 = fStart-fHyp;
            if (2 * (fStart-2*f+fHyp) * t1*t1 - maxDelta * t2*t2 < 0) {
                f = LineSearch(func, x, overallDir, xTol*100);
                // Discard direction of largest decrease.
                for (int i = 0; i < N; i++) {
                    dirSet[argMaxDelta][i] = dirSet[N-1][i];
                    dirSet[N-1][i]         = overallDir[i];
                }
            }
        }
    }

    return f;
}

////////////////////////////////////////////////////////////////////////////////

template<class Function>
double
LineSearch(Function &func, DoubleVector &x, DoubleVector &dir,
           double xTol=1e-3) {
    double alphaA = 0.0, alphaB = 1.0, alphaC, alphaMin;
    double fA, fB, fC, fMin;
    int    numIter;

    Function1D<Function> func1d(func, x, dir);
    Bracket(func1d, alphaA, alphaB, alphaC, fA, fB, fC, numIter);
    fMin = Brent(func1d, alphaA, alphaB, alphaC, alphaMin, numIter, xTol);
    for (size_t i = 0; i < x.length(); i++) {
        dir[i] *= alphaMin;
        x[i] += dir[i];
    }
    return fMin;
}

////////////////////////////////////////////////////////////////////////////////

template<class Function1D>
double
Brent(Function1D &func, double xa, double xb, double xc,
      double &xMin, int &numIter, double tol=1.48e-8, int maxIter=500) {

    const double cGold  = 0.3819660;  // Golden ratio
    const double minTol = 1e-11;

    double fu, fv, fw, fx;
    double u, v, w, x;
    double xDelta = 0, d = 0;
    double a = std::min(xa, xc);
    double b = std::max(xa, xc);
    w  = v  = x  = xb;
    fw = fv = fx = func(x);

    for (numIter = 0; numIter < maxIter; numIter++) {
        double xMid = 0.5 * (a + b);
        double tol1 = tol * fabs(x) + minTol;
        double tol2 = 2 * tol1;
        double xDeltaTemp;

        // Check for convergence.
        if (fabs(x - xMid) < (tol2 - 0.5 * (b - a)))
            break;

        if (fabs(xDelta) <= tol1) {
            // Golden section step.
            xDelta = ((x >= xMid) ? a : b) - x;
            d = cGold * xDelta;
        } else {
            // Parabolic step.
            double r = (x - w) * (fx - fv);
            double q = (x - v) * (fx - fw);
            double p = (x - v) * q - (x - w) * r;
            q = 2 * (q - r);
            if (q > 0) p = -p;
            q = fabs(q);
            xDeltaTemp = xDelta;
            xDelta = d;
            // Check parabolic fit.
            if ((fabs(p) < fabs(0.5 * q * xDeltaTemp)) &&
                (p > q * (a - x)) && (p < q * (b - x))) {
                // Parabolic step is useful.
                d = p / q;
                u = x + d;
                if (u - a < tol2 || b - u < tol2)
                    d = (xMid - x >= 0) ? tol1 : -tol1;
            } else {
                // Golden section step.
                xDelta = (x >= xMid) ? a - x : b - x;
                d = cGold * xDelta;
            }
        }
        // Update by at least tol1.
        if (fabs(d) >= tol1)
            u = x + d;
        else if (d >= 0)
            u = x + tol1;
        else
            u = x - tol1;

        // Calculate new function value.
        fu = func(u);
        if (fu < fx) {
            if (u >= x) a = x;  else b = x;
            v  = w;   w  = x;   x  = u;
            fv = fw;  fw = fx;  fx = fu;
        } else {
            if (u < x) a = u;  else b = u;
            if (fu <= fw || w == x) {
                v = w;  w = u;  fv = fw;  fw = fu;
            } else if (fu <= fv || v == x || v == w) {
                v = u;  fv = fu;
            }
        }
    }

    xMin = x;
    return fx;
}

////////////////////////////////////////////////////////////////////////////////

template<class Function1D>
inline void
Bracket(Function1D &func, double &xa, double &xb, double &xc,
        double &fa, double &fb, double &fc, int &numIter,
        double growLimit=110.0, int maxIter=1000) {
    const double gold    = 1.618034;
    const double epsilon = 1e-21;

    fa = func(xa);  fb = func(xb);
    if (fa < fb) {
        std::swap(xa, xb);
        std::swap(fa, fb);
    }
    xc = xb + gold * (xb - xa);
    fc = func(xc);

    numIter = 0;
    while (fc < fb) {
        double t1    = (xb - xa) * (fb - fc);
        double t2    = (xb - xc) * (fb - fa);
        double val   = t2 - t1;
        double denom = 2 * ((fabs(val) < epsilon) ? epsilon : val);
        double w     = xb - ((xb - xc) * t2 - (xb - xa) * t1) / denom;
        double wLim  = xb + growLimit * (xc - xb);
        double fw;

        if (++numIter > maxIter)
            break;
        if ((w - xc) * (xb - w) > 0) {
            fw = func(w);
            if (fw < fc) {
                xa = xb;  xb = w;  fa = fb;  fb = fw;
                break;
            } else if (fw > fb) {
                xc = w;  fc = fw;
                break;
            }
            w = xc + gold * (xc - xb);
            fw = func(w);
        } else if ((w - wLim) * (xc - w) > 0) {
            fw = func(w);
            if (fw < fc) {
                xb = xc;  xc = w;   w = xc + gold * (xc - xb);
                fb = fc;  fc = fw;  fw = func(w);
            }
        } else {
            w = xc + gold * (xc - xb);
            fw = func(w);
        }
        xa = xb;  xb = xc;  xc = w;
        fa = fb;  fb = fc;  fc = fw;
    }
}

#endif // POWELL_H
