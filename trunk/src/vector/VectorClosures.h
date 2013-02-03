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

#ifndef VECTORCLOSURES_H
#define VECTORCLOSURES_H

#include <cassert>
#include "Operations.h"
#include "Vector.h"
#include "Traits.h"

namespace mitlm {

////////////////////////////////////////////////////////////////////////////////

template <typename V, typename I>
class IndirectVectorClosure : public Vector<IndirectVectorClosure<V, I> > {
    typedef IndirectVectorClosure<V, I> SelfT;

public:
    typedef typename TypeInfo<V>::ElementType ElementType;

    class ConstIterator {
    public:
        ConstIterator(typename Ref<V>::Type v, typename I::ConstIterator i)
          : _v(v), _i(i) { }
        ConstIterator & operator++()      { ++_i; return *this; }
        ElementType     operator*() const { return _v[*_i]; }
        bool operator==(const ConstIterator &i) const { return i._i == _i; }
        bool operator!=(const ConstIterator &i) const { return !operator==(i); }

    private:
        typename Ref<V>::Type     _v;
        typename I::ConstIterator _i;
    };

    IndirectVectorClosure(typename Ref<V>::Type v, typename Ref<I>::Type i)
      : _v(v.impl()), _i(i.impl()) { }
    size_t        length() const { return _i.length(); }
    ConstIterator begin() const  { return ConstIterator(_v, _i.begin()); }
    ConstIterator end() const    { return ConstIterator(_v, _i.end()); }
    ElementType   operator[](size_t index) const { return _v[_i[index]]; }

    template <typename I2>
    const IndirectVectorClosure<SelfT, I2> operator[](const Vector<I2> &x) const
    { return IndirectVectorClosure<SelfT, I2>(*this, x.impl()); }

private:
    typename Ref<typename V::Impl>::Type _v;
    typename Ref<typename I::Impl>::Type _i;
};

////////////////////////////////////////////////////////////////////////////////

template <typename V, typename I>
struct TypeInfo<IndirectVectorClosure<V, I> > {
    typedef IndirectVectorClosure<V, I> Impl;
    typedef typename V::ElementType     ElementType;
};

////////////////////////////////////////////////////////////////////////////////

template <typename Op, typename V>
class UnaryVectorClosure : public Vector<UnaryVectorClosure<Op, V> > {
    typedef UnaryVectorClosure<Op,V> SelfT;

public:
    typedef typename TypeInfo<SelfT>::ElementType ElementType;

    class ConstIterator {
    public:
        ConstIterator(typename V::ConstIterator i) : _i(i) { }
        ConstIterator & operator++() { ++_i; return *this; }
        ElementType     operator*() const { return Op::Eval(*_i); }
        bool operator==(const ConstIterator &i) const { return i._i == _i; }
        bool operator!=(const ConstIterator &i) const { return !operator==(i); }

    private:
        typename V::ConstIterator _i;
    };

    UnaryVectorClosure(typename Ref<V>::Type v) : _v(v.impl()) { }
    size_t        length() const { return _v.length(); }
    ConstIterator begin() const  { return ConstIterator(_v.begin()); }
    ConstIterator end() const    { return ConstIterator(_v.end()); }
    ElementType   operator[](size_t i) const { return Op::Eval(_v[i]); }

    template <typename I>
    const IndirectVectorClosure<SelfT, I> operator[](const Vector<I> &x) const
    { return IndirectVectorClosure<SelfT, I>(*this, x.impl()); }

private:
    typename Ref<typename V::Impl>::Type _v;
};

////////////////////////////////////////////////////////////////////////////////

template <typename Op, typename V>
struct TypeInfo<UnaryVectorClosure<Op, V> > {
    typedef UnaryVectorClosure<Op, V> Impl;
    typedef typename V::ElementType   ElementType;
};

template <typename V>
struct TypeInfo<UnaryVectorClosure<OpDouble, V> > {
    typedef UnaryVectorClosure<OpDouble, V> Impl;
    typedef double                          ElementType;
};

template <typename V>
struct TypeInfo<UnaryVectorClosure<OpFloat, V> > {
    typedef UnaryVectorClosure<OpFloat, V> Impl;
    typedef float                          ElementType;
};

template <typename V>
struct TypeInfo<UnaryVectorClosure<OpInt, V> > {
    typedef UnaryVectorClosure<OpInt, V> Impl;
    typedef int                          ElementType;
};

////////////////////////////////////////////////////////////////////////////////

template <typename Op, typename L, typename R>
class VectorClosure : public Vector<VectorClosure<Op, L, R> > {
    typedef VectorClosure<Op, L, R> SelfT;

public:
    typedef typename TypeInfo<SelfT>::ElementType ElementType;

    class ConstIterator {
    public:
        ConstIterator(typename L::ConstIterator l, typename R::ConstIterator r)
            : _l(l), _r(r) { }
        ConstIterator & operator++()      { ++_l; ++_r; return *this; }
        ElementType     operator*() const { return Op::Eval(*_l, *_r); }
        bool operator==(const ConstIterator &i) const
                                          { return i._l == _l && i._r == _r; }
        bool operator!=(const ConstIterator &i) const { return !operator==(i); }

    private:
        typename L::ConstIterator _l;
        typename R::ConstIterator _r;
    };

    VectorClosure(typename Ref<L>::Type l, typename Ref<R>::Type r)
      : _l(l.impl()), _r(r.impl()) { }
    size_t length() const       { return std::max(_l.length(), _r.length()); }
    ConstIterator begin() const { return ConstIterator(_l.begin(),_r.begin()); }
    ConstIterator end() const   { return ConstIterator(_l.end(), _r.end()); }
    ElementType   operator[](size_t i) const { return Op::Eval(_l[i], _r[i]); }

    template <typename I>
    const IndirectVectorClosure<SelfT, I> operator[](const Vector<I> &x) const
    { return IndirectVectorClosure<SelfT, I>(*this, x.impl()); }

private:
    typename Ref<typename L::Impl>::Type _l;
    typename Ref<typename R::Impl>::Type _r;
};

////////////////////////////////////////////////////////////////////////////////

template <typename L, typename R>
struct VPromotion {
    typedef typename Promotion<typename Vector<L>::ElementType,
                               typename Vector<R>::ElementType>::Type Type;
};

////////////////////////////////////////////////////////////////////////////////

template <typename Op, typename L, typename R>
struct TypeInfo<VectorClosure<Op, L, R> > {
    typedef VectorClosure<Op, L, R>         Impl;
    typedef typename VPromotion<L, R>::Type ElementType;
};

////////////////////////////////////////////////////////////////////////////////

template <typename C, typename T, typename F>
class CondVectorClosure : public Vector<CondVectorClosure<C, T, F> > {
    typedef CondVectorClosure<C, T, F> SelfT;

public:
    typedef typename TypeInfo<SelfT>::ElementType ElementType;

    class ConstIterator {
    public:
        ConstIterator(typename C::ConstIterator c, typename T::ConstIterator t,
                      typename F::ConstIterator f)
            : _c(c), _t(t), _f(f) { }
        ConstIterator & operator++()      { ++_c; ++_t; ++_f; return *this; }
        ElementType     operator*() const { return *_c ? *_t : *_f; }
        bool operator==(const ConstIterator &i) const
                                          { return i._c == _c; }
        bool operator!=(const ConstIterator &i) const { return !operator==(i); }

    private:
        typename C::ConstIterator _c;
        typename T::ConstIterator _t;
        typename F::ConstIterator _f;
    };

    CondVectorClosure(typename Ref<C>::Type c, typename Ref<T>::Type t,
                      typename Ref<F>::Type f) : _c(c), _t(t), _f(f) { }
    size_t        length() const { return _c.length(); }
    ConstIterator begin() const
      { return ConstIterator(_c.begin(), _t.begin(), _f.begin()); }
    ConstIterator end() const
      { return ConstIterator(_c.end(), _t.end(), _f.end()); }
    ElementType   operator[](size_t i) const { return _c[i] ? _t[i] : _f[i]; }

private:
    typename Ref<typename C::Impl>::Type _c;
    typename Ref<typename T::Impl>::Type _t;
    typename Ref<typename F::Impl>::Type _f;
};

////////////////////////////////////////////////////////////////////////////////

template <typename C, typename T, typename F>
struct TypeInfo<CondVectorClosure<C, T, F> > {
    typedef CondVectorClosure<C, T, F>      Impl;
    typedef typename VPromotion<T, F>::Type ElementType;
};

////////////////////////////////////////////////////////////////////////////////

template <typename V, typename M>
class MaskedVectorClosure {
public:
    MaskedVectorClosure(V &v, typename Ref<M>::Type m)
      : _v(v.impl()), _m(m.impl()) { }
    size_t                               length() const { return _v.length(); }
    typename V::Impl &                   vector()       { return _v; }
    typename Ref<typename M::Impl>::Type mask() const   { return _m; }

    template <typename RHS>
    void operator=(const Vector<RHS> &rhs) { MaskAssign(_m, rhs.impl(), _v); }
    void set(typename V::ElementType value)
      { for (size_t i = 0; i < _v.length(); i++) if (_m[i]) _v[i] = 0; }

private:
    typename V::Impl &                   _v;
    typename Ref<typename M::Impl>::Type _m;
};

}

#endif // VECTORCLOSURES_H
