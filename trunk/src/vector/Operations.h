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

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// Boolean Operations

struct OpNot {
    template <typename T>
    static bool Eval(T v) { return !v; }
};

struct OpEqual {
    template <typename L, typename R>
    static bool Eval(L a, R b) { return a == b; }
};

struct OpNotEqual {
    template <typename L, typename R>
    static bool Eval(L a, R b) { return a != b; }
};

struct OpLessThan {
    template <typename L, typename R>
    static bool Eval(L a, R b) { return a < b; }
};

struct OpLessEqual {
    template <typename L, typename R>
    static bool Eval(L a, R b) { return a <= b; }
};

////////////////////////////////////////////////////////////////////////////////
// Arithmetic Operations

struct OpNeg {
    template <typename T>
    static T Eval(T v) { return -v; }
};

struct OpAdd {
    template <typename L, typename R>
    static typename Promotion<L, R>::Type Eval(L a, R b) { return a+b; }
};

struct OpSub {
    template <typename L, typename R>
    static typename Promotion<L, R>::Type Eval(L a, R b) { return a-b; }
};

struct OpMult {
    template <typename L, typename R>
    static typename Promotion<L, R>::Type Eval(L a, R b) { return a*b; }
};

struct OpDiv {
    template <typename L, typename R>
    static typename Promotion<L, R>::Type Eval(L a, R b) { return a/b; }
};

////////////////////////////////////////////////////////////////////////////////
// Advanced Operations

struct OpMin {
    template <typename T> static T Eval(T a, T b) { return std::min(a, b); }
};

struct OpMax {
    template <typename T> static T Eval(T a, T b) { return std::max(a, b); }
};

struct OpAbs {
    template <typename T> static T Eval(T v) { return fabs(v); }
};

struct OpLog {
    template <typename T> static T Eval(T v) { return log(v); }
};

struct OpLog10 {
    template <typename T> static T Eval(T v) { return log10(v); }
};

struct OpExp {
    template <typename T> static T Eval(T v) { return exp(v); }
};

struct OpPow {
    template <typename L, typename R>
    static typename Promotion<L, R>::Type
    Eval(L a, R b) {
        typedef typename Promotion<L, R>::Type ResultType;
        return pow((ResultType)a, (ResultType)b);
    }
};

struct OpIsNan {
	template <typename T> static T Eval(T v) { return std::isnan(v); }
};

struct OpIsInf {
    template <typename T> static T Eval(T v) { return std::isinf(v); }
};

struct OpIsFinite {
    template <typename T> static T Eval(T v) { return std::isfinite(v); }
};

////////////////////////////////////////////////////////////////////////////////
// Type Casting Operations

struct OpDouble {
    template <typename T> static double Eval(T v) { return (double)v; }
};

struct OpFloat {
    template <typename T> static float Eval(T v) { return (float)v; }
};

struct OpInt {
    template <typename T> static int Eval(T v) { return static_cast<int>(v); }
};

#endif // OPERATIONS_H
