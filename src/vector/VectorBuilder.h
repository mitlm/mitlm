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

#ifndef VECTORBUILDER_H
#define VECTORBUILDER_H

#include <algorithm>
#include "util/BitOps.h"
#include "Vector.h"
#include "DenseVector.h"

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class VectorBuilder : public Vector<VectorBuilder<T> > {
public:
    typedef T         ElementType;
    typedef const T * ConstIterator;

    VectorBuilder(size_t capacity=16) : _length(0) {
        _vector.resize(std::max(capacity, (size_t)16));
    }

    void append(typename Ref<T>::Type value, size_t count=1) {
        if (length() + count > _vector.length())
            _vector.resize(nextPowerOf2(length() + count - 1));
        std::fill_n(_vector.begin() + _length, count, value);
        _length += count;
    }

    template <typename RHS>
    void append(const Vector<RHS> &rhs) {
        size_t newLength = _length + rhs.impl().length();
        if (newLength > _vector.length())
            _vector.resize(nextPowerOf2(newLength - 1));

        typename Vector<RHS>::Impl::ConstIterator rBegin = rhs.impl().begin();
        typename Vector<RHS>::Impl::ConstIterator rEnd   = rhs.impl().end();
        T *lBegin  = _vector.begin() + _length;
        while (rBegin != rEnd)
            *lBegin++ = *rBegin++;
        _length = newLength;
    }

    size_t length() const       { return _length; }
    ConstIterator begin() const { return _vector.begin(); }
    ConstIterator end() const   { return _vector.end(); }

protected:
    size_t         _length;
    DenseVector<T> _vector;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TypeInfo<VectorBuilder<T> > {
    typedef VectorBuilder<T> Impl;
    typedef T                ElementType;
};

#endif // VECTORBUILDER_H
