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

#ifndef TRAITS_H
#define TRAITS_H

namespace mitlm {
template <typename A> struct Ref { typedef const A &Type; };
template <> struct Ref<void>     { typedef void Type; };
template <> struct Ref<short>    { typedef short Type; };
template <> struct Ref<int>      { typedef int Type; };
template <> struct Ref<long>     { typedef long Type; };
template <> struct Ref<float>    { typedef float Type; };
template <> struct Ref<double>   { typedef double Type; };

template <typename A, typename B> struct Promotion { };
template <typename A> struct Promotion<A, A> { typedef A Type; };
template <> struct Promotion<short,  int>     { typedef int Type; };
template <> struct Promotion<int,    short>   { typedef int Type; };
template <> struct Promotion<short,  long>    { typedef long Type; };
template <> struct Promotion<long,   short>   { typedef long Type; };
template <> struct Promotion<int,    long>    { typedef long Type; };
template <> struct Promotion<long,   int>     { typedef long Type; };
template <> struct Promotion<short,  float>   { typedef float Type; };
template <> struct Promotion<float,  short>   { typedef float Type; };
template <> struct Promotion<int,    float>   { typedef float Type; };
template <> struct Promotion<float,  int>     { typedef float Type; };
template <> struct Promotion<short,  double>  { typedef double Type; };
template <> struct Promotion<double, short>   { typedef double Type; };
template <> struct Promotion<int,    double>  { typedef double Type; };
template <> struct Promotion<double, int>     { typedef double Type; };
template <> struct Promotion<long,   double>  { typedef double Type; };
template <> struct Promotion<double, long>    { typedef double Type; };
template <> struct Promotion<float,  double>  { typedef double Type; };
template <> struct Promotion<double, float>   { typedef double Type; };
template <> struct Promotion<long,   float>   { typedef double Type; };
template <> struct Promotion<float,  long>    { typedef double Type; };

}

#endif // TRAITS_H
