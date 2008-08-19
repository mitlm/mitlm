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

#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include "vector/DenseVector.h"
#include "vector/VectorBuilder.h"
#include "vector/VectorOps.h"

////////////////////////////////////////////////////////////////////////////////

// Type aliases.
typedef unsigned char byte;
typedef unsigned int  uint;

// Defines the size of basic types.
typedef int    VocabIndex;
typedef int    NgramIndex;
typedef int    Count;
typedef float  LProb;
typedef double Prob;
typedef double Param;

// Vector aliases.
typedef DenseVector<const char *> StrVector;
typedef DenseVector<byte>         BitVector;
typedef DenseVector<byte>         ByteVector;
typedef DenseVector<int>          IntVector;
typedef DenseVector<size_t>       SizeVector;
typedef DenseVector<float>        FloatVector;
typedef DenseVector<double>       DoubleVector;
typedef DenseVector<VocabIndex>   VocabVector;
typedef DenseVector<Count>        CountVector;
typedef DenseVector<NgramIndex>   IndexVector;
typedef DenseVector<Prob>         ProbVector;
typedef DenseVector<Param>        ParamVector;

typedef std::vector<DoubleVector> FeatureVectors;


#endif // TYPES_H
