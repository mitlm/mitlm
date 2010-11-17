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

#ifndef VECTOROPS_H
#define VECTOROPS_H

#include "Scalar.h"
#include "Vector.h"
#include "VectorClosures.h"
#include <limits>

////////////////////////////////////////////////////////////////////////////////
// Boolean Operations

template <typename V> const UnaryVectorClosure<OpNot, V>
operator!(const Vector<V> &x)
{ return UnaryVectorClosure<OpNot, V>(x.impl()); }

template <typename V, typename T> const VectorClosure<OpEqual, V, Scalar<T> >
operator==(const Vector<V> &x, T c)
{ return VectorClosure<OpEqual, V, Scalar<T> >(x.impl(), c); }

template <typename V, typename T> const VectorClosure<OpNotEqual, V, Scalar<T> >
operator!=(const Vector<V> &x, T c)
{ return VectorClosure<OpNotEqual, V, Scalar<T> >(x.impl(), c); }

template <typename V, typename T> const VectorClosure<OpLessThan, V, Scalar<T> >
operator<(const Vector<V> &x, T c)
{ return VectorClosure<OpLessThan, V, Scalar<T> >(x.impl(), c); }

template <typename V, typename T> const VectorClosure<OpLessEqual,V, Scalar<T> >
operator<=(const Vector<V> &x, T c)
{ return VectorClosure<OpLessEqual, V, Scalar<T> >(x.impl(), c); }

template <typename V, typename T> const VectorClosure<OpLessThan, Scalar<T>, V>
operator>(const Vector<V> &x, T c)
{ return VectorClosure<OpLessThan, Scalar<T>, V>(c, x.impl()); }

template <typename V, typename T> const VectorClosure<OpLessEqual,Scalar<T>, V>
operator>=(const Vector<V> &x, T c)
{ return VectorClosure<OpLessEqual, Scalar<T>, V>(c, x.impl()); }

template <typename V> bool
anyTrue(const Vector<V> &x) {
    for (typename V::ConstIterator p=x.impl().begin(); p!=x.impl().end(); ++p)
        if (*p) return true;
    return false;
}

template <typename V> bool
allTrue(const Vector<V> &x) {
    for (typename V::ConstIterator p=x.impl().begin(); p!=x.impl().end(); ++p)
        if (!*p) return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Arithmetic Operations

template <typename V> const UnaryVectorClosure<OpNeg, V>
operator-(const Vector<V> &x)
{ return UnaryVectorClosure<OpNeg, V>(x.impl()); }


template <typename V> const VectorClosure<OpAdd, V, typename V::ScalarType>
operator+(typename V::ElementType c, const Vector<V> &x)
{ return VectorClosure<OpAdd, V, typename V::ScalarType>(x.impl(), c); }

template <typename V> const VectorClosure<OpSub, typename V::ScalarType, V>
operator-(typename V::ElementType c, const Vector<V> &x)
{ return VectorClosure<OpSub, typename V::ScalarType, V>(c, x.impl()); }

template <typename V> const VectorClosure<OpMult, V, typename V::ScalarType>
operator*(typename V::ElementType c, const Vector<V> &x)
{ return VectorClosure<OpMult, V, typename V::ScalarType>(x.impl(), c); }

template <typename V> const VectorClosure<OpDiv, typename V::ScalarType, V>
operator/(typename V::ElementType c, const Vector<V> &x)
{ return VectorClosure<OpDiv, typename V::ScalarType, V>(c, x.impl()); }


template <typename V> const VectorClosure<OpAdd, V, typename V::ScalarType>
operator+(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpAdd, V, typename V::ScalarType>(x.impl(), c); }

template <typename V> const VectorClosure<OpSub, V, typename V::ScalarType>
operator-(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpSub, V, typename V::ScalarType>(x.impl(), c); }

template <typename V> const VectorClosure<OpMult, V, typename V::ScalarType>
operator*(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpMult, V, typename V::ScalarType>(x.impl(), c); }

template <typename V> const VectorClosure<OpDiv, V, typename V::ScalarType>
operator/(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpDiv, V, typename V::ScalarType>(x.impl(), c); }


template <typename L, typename R> const VectorClosure<OpAdd, L, R>
operator+(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpAdd, L, R>(x.impl(), y.impl()); }

template <typename L, typename R> const VectorClosure<OpSub, L, R>
operator-(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpSub, L, R>(x.impl(), y.impl()); }

template <typename L, typename R> const VectorClosure<OpMult, L, R>
operator*(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpMult, L, R>(x.impl(), y.impl()); }

template <typename L, typename R> const VectorClosure<OpDiv, L, R>
operator/(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpDiv, L, R>(x.impl(), y.impl()); }

////////////////////////////////////////////////////////////////////////////////
// Special Operations

template <typename V> typename V::ElementType
sum(const Vector<V> &x) {
    typename V::ElementType total = 0;
    for (typename V::ConstIterator p=x.impl().begin(); p!=x.impl().end(); ++p)
        total += *p;
    return total;
}

template <typename V> typename V::ElementType
min(const Vector<V> &x) {
    typedef typename V::ElementType ElementType;
    ElementType v = std::numeric_limits<ElementType>::max();
    for (typename V::ConstIterator p=x.impl().begin(); p!=x.impl().end(); ++p)
        if (*p < v) v = *p;
    return v;
}

template <typename V> typename V::ElementType
max(const Vector<V> &x) {
    typedef typename V::ElementType ElementType;
    ElementType v = std::numeric_limits<ElementType>::min();
    for (typename V::ConstIterator p=x.impl().begin(); p!=x.impl().end(); ++p)
        if (*p > v) v = *p;
    return v;
}

template <typename L, typename R> const typename VPromotion<L, R>::Type
dot(const Vector<L> &x, const Vector<R> &y) {
    assert(x.impl().length() == y.impl().length());
    typename VPromotion<L, R>::Type result = 0;
    typename L::ConstIterator pX    = x.impl().begin();
    typename L::ConstIterator pXEnd = x.impl().end();
    typename R::ConstIterator pY    = y.impl().begin();
    while (pX != pXEnd)
        result += (*pX++) * (*pY++);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Advanced Operations

template <typename L, typename R> const VectorClosure<OpMin, L, R>
min(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpMin, L, R>(x.impl(), y.impl()); }

template <typename V> const VectorClosure<OpMin, V, typename V::ScalarType>
min(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpMin, V, typename V::ScalarType>(x.impl(), c); }

template <typename L, typename R> const VectorClosure<OpMax, L, R>
max(const Vector<L> &x, const Vector<R> &y)
{ return VectorClosure<OpMax, L, R>(x.impl(), y.impl()); }

template <typename V> const VectorClosure<OpMax, V, typename V::ScalarType>
max(const Vector<V> &x, typename V::ElementType c)
{ return VectorClosure<OpMax, V, typename V::ScalarType>(x.impl(), c); }


template <typename V> const UnaryVectorClosure<OpAbs, V>
abs(const Vector<V> &x) { return UnaryVectorClosure<OpAbs, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpLog, V>
log(const Vector<V> &x) { return UnaryVectorClosure<OpLog, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpLog10, V>
log10(const Vector<V> &x) { return UnaryVectorClosure<OpLog10, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpExp, V>
exp(const Vector<V> &x) { return UnaryVectorClosure<OpExp, V>(x.impl()); }

template <typename V, typename T> const VectorClosure<OpPow, V, Scalar<T> >
pow(const Vector<V> &x, T c)
{ return VectorClosure<OpPow, V, Scalar<T> >(x.impl(), c); }

template <typename V> const UnaryVectorClosure<OpIsNan, V>
isnan(const Vector<V> &x) { return UnaryVectorClosure<OpIsNan, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpIsInf, V>
isinf(const Vector<V> &x){return UnaryVectorClosure<OpIsInf,V>(x.impl());}

template <typename V> const UnaryVectorClosure<OpIsFinite, V>
isfinite(const Vector<V> &x){return UnaryVectorClosure<OpIsFinite,V>(x.impl());}

////////////////////////////////////////////////////////////////////////////////
// Type Casting Operations

template <typename V> const UnaryVectorClosure<OpDouble, V>
asDouble(const Vector<V> &x)
{ return UnaryVectorClosure<OpDouble, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpFloat, V>
asFloat(const Vector<V> &x)
{ return UnaryVectorClosure<OpFloat, V>(x.impl()); }

template <typename V> const UnaryVectorClosure<OpInt, V>
asInt(const Vector<V> &x)
{ return UnaryVectorClosure<OpInt, V>(x.impl()); }

////////////////////////////////////////////////////////////////////////////////
// Bin Operations

template <typename I, typename T>
void BinCount(const Vector<I> &i, DenseVector<T> &result) {
    typename I::ConstIterator iBegin = i.impl().begin();
    typename I::ConstIterator iEnd = i.impl().end();
    while (iBegin != iEnd) {
        size_t index = *iBegin;
        assert(index < result.length());
        result[index]++;
        ++iBegin;
    }
}

template <typename I, typename T>
void BinClippedCount(const Vector<I> &i, DenseVector<T> &result) {
    typename I::ConstIterator iBegin = i.impl().begin();
    typename I::ConstIterator iEnd = i.impl().end();
    while (iBegin != iEnd) {
        size_t index = *iBegin;
        if (index < result.length())
            result[index]++;
        ++iBegin;
    }
}

template <typename I, typename W, typename T>
void BinWeight(const Vector<I> &i, const Vector<W> &w, DenseVector<T> &result) {
    assert(i.impl().length() == w.impl().length());
    typename I::ConstIterator iBegin = i.impl().begin();
    typename I::ConstIterator iEnd = i.impl().end();
    typename W::ConstIterator wBegin = w.impl().begin();
    while (iBegin != iEnd) {
        size_t index = *iBegin;
        assert(index < result.length());
        result[index] += *wBegin;
        ++iBegin; ++wBegin;
    }
}

template <typename I, typename W, typename V, typename M>
void BinWeight(const Vector<I> &i, const Vector<W> &w,
               MaskedVectorClosure<V, M> &result) {
    assert(i.impl().length() == w.impl().length());
    assert(result.mask().impl().length() == result.vector().impl().length());
    typename I::ConstIterator iBegin = i.impl().begin();
    typename I::ConstIterator iEnd = i.impl().end();
    typename W::ConstIterator wBegin = w.impl().begin();
    while (iBegin != iEnd) {
        size_t index = *iBegin;
        assert(index < result.length());
        if (result.mask()[index])
            result.vector()[index] += *wBegin;
        ++iBegin; ++wBegin;
    }
}

template <typename I, typename T, typename R>
void BinLookup(const Vector<I> &i, const DenseVector<T> &t, DenseVector<R> &r,
               T def=T()) {
    assert(i.impl().length() == r.length());
    typename I::ConstIterator         iBegin = i.impl().begin();
    typename I::ConstIterator         iEnd = i.impl().end();
    typename DenseVector<R>::Iterator rBegin = r.begin();
    while (iBegin != iEnd) {
        size_t index = *iBegin++;
        *rBegin++ = (index >= t.length()) ? def : t[index];
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename C, typename T, typename F> CondVectorClosure<C, T, F>
CondExpr(const Vector<C> &cond, const Vector<T> &t, const Vector<F> &f)
{ return CondVectorClosure<C, T, F>(cond.impl(), t.impl(), f.impl()); }

template <typename C, typename T, typename F> CondVectorClosure<C, Scalar<T>, F>
CondExpr(const Vector<C> &c, T t, const Vector<F> &f)
{ return CondVectorClosure<C, Scalar<T>, F>(c.impl(), t, f.impl()); }

////////////////////////////////////////////////////////////////////////////////
// Filtered Operations

template <typename M, typename I, typename O>
void IndexAssign(const Vector<M> &index, const Vector<I> &input,
                Vector<O> &output) {
    output.impl().reset(input.impl().length());
    typename M::ConstIterator begin = index.impl().begin();
    typename M::ConstIterator end = index.impl().end();
    while (begin != end) {
        size_t index = *begin;
        assert(index < input.impl().length());
        output.impl()[index] = input.impl()[index];
        ++begin;
    }
}

template <typename M, typename I, typename O>
void MaskAssign(const Vector<M> &mask, const Vector<I> &input,
                Vector<O> &output) {
    assert(input.impl().length() == output.impl().length());
    assert(mask.impl().length() == input.impl().length());
    typename M::ConstIterator pM    = mask.impl().begin();
    typename M::ConstIterator pMEnd = mask.impl().end();
    for (size_t i = 0; pM != pMEnd; ++pM, ++i)
        if (*pM) output.impl()[i] = input.impl()[i];
}

#endif // VECTOROPS_H
