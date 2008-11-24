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

#ifndef DENSEVECTOR_H
#define DENSEVECTOR_H

#include <iostream>
#include <vector>
#include "Vector.h"
#include "VectorClosures.h"
#include "Range.h"
#include "Traits.h"

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class DenseVector: public Vector<DenseVector<T> > {
public:
    typedef T         ElementType;
    typedef const T * ConstIterator;
    typedef T *       Iterator;

    DenseVector(size_t length = 0);
    DenseVector(size_t length, T value);
    DenseVector(const Range &r);
    DenseVector(const DenseVector<T> &rhs);
    DenseVector(const DenseVector<T> &rhs, bool clone);
    template <typename RHS> DenseVector(const Vector<RHS> &rhs);
    ~DenseVector();

    DenseVector<T> &                       operator=(T value);
    DenseVector<T> &                       operator=(const Range &r);
    DenseVector<T> &                       operator=(const DenseVector<T> &rhs);
    DenseVector<T> &                       operator=(const std::vector<T> &rhs);
    template <typename RHS> DenseVector<T> &operator=(const Vector<RHS> &rhs);
    template <typename RHS> DenseVector<T> &operator+=(const Vector<RHS> &rhs);
    template <typename RHS> DenseVector<T> &operator-=(const Vector<RHS> &rhs);
    template <typename RHS> DenseVector<T> &operator*=(const Vector<RHS> &rhs);
    template <typename RHS> DenseVector<T> &operator/=(const Vector<RHS> &rhs);
    DenseVector<T> &                       operator+=(T alpha);
    DenseVector<T> &                       operator-=(T alpha);
    DenseVector<T> &                       operator*=(T alpha);
    DenseVector<T> &                       operator/=(T alpha);
    const T &                              operator[](size_t index) const;
    T &                                    operator[](size_t index);
    const DenseVector<T>                   operator[](const Range &r) const;
    DenseVector<T>                         operator[](const Range &r);

    template <typename RHS>
    const IndirectVectorClosure<DenseVector<T>, typename RHS::Impl>
    operator[](const Vector<RHS> &x) const;

    template <typename RHS>
    IndirectVectorClosure<DenseVector<T>, typename RHS::Impl>
    operator[](const Vector<RHS> &x);

    template <typename M>
    MaskedVectorClosure<DenseVector<T>, typename M::Impl>
    masked(const Vector<M> &mask);

    void reset(size_t length);
    void reset(size_t length, T value);
    void resize(size_t length);
    void resize(size_t length, T value);
    void swap(DenseVector<T> &v);
    void set(T value);
    void attach(const DenseVector<T> &v);

    size_t        length() const { return _length; }
    ConstIterator begin() const  { return _data; }
    ConstIterator end() const    { return _data + _length; }
    const T *     data() const   { return _data; }
    Iterator      begin()        { return _data; }
    Iterator      end()          { return _data + _length; }
    T *           data()         { return _data; }

private:
    DenseVector(size_t length, T *data, void *storage);
    void _allocate();
    void _release();

    size_t _length;
    T *    _data;
    void * _storage;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TypeInfo<DenseVector<T> > {
    typedef DenseVector<T> Impl;
    typedef T              ElementType;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
std::ostream &operator<<(std::ostream &o, const DenseVector<T> &x);

template <>
std::ostream &operator<<(std::ostream &o, const DenseVector<unsigned char> &x);

template <typename T>
void WriteVector(FILE *out, const DenseVector<T> &x);

template <typename T>
void ReadVector(FILE *in, DenseVector<T> &x);

////////////////////////////////////////////////////////////////////////////////

#include "DenseVector.tcc"

#endif // DENSEVECTOR_H
