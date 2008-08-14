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

#include <cassert>
#include <algorithm>
#include "util/RefCounter.h"
#include "util/Logger.h"
#include "util/FastIO.h"

////////////////////////////////////////////////////////////////////////////////

// data == NULL
//   Empty zero-length vector.
// data != NULL && storage == _data
//   Regular vector.  Could also be a prefix view.
//   AddRefCount on copy construct.
//   ReleaseRefCount/Free on detach.
// data != NULL && storage != _data
//   View into another DenseVector.
//   AddRefCount on attach.
//   ReleaseRefCount/Free on detach.
// data != NULL && storage == NULL
//   View into memory managed elsewhere.
//   No specific action on attach or detach.

template <typename T>
DenseVector<T>::DenseVector(size_t length)
    : _length(length), _data(0), _storage(0)
{
    _allocate();
}

template <typename T>
DenseVector<T>::DenseVector(size_t length, T value)
    : _length(length), _data(0), _storage(0)
{
    _allocate();
    std::fill_n(_data, _length, value);
}

template <typename T>
DenseVector<T>::DenseVector(const Range &r)
    : _length(r.length()), _data(0), _storage(0)
{
    _allocate();
    int v = r.beginIndex();
    for (Iterator p = begin(); p != end(); ++p, ++v)
        *p = v;
}

template <typename T>
DenseVector<T>::DenseVector(const DenseVector<T> &rhs)
    : _length(rhs._length), _data(rhs._data), _storage(rhs._storage)
{
    if (_storage)
        RefCounter.attach(_storage);
}

template <typename T>
DenseVector<T>::DenseVector(const DenseVector<T> &rhs, bool clone)
    : _length(rhs._length), _data(0), _storage(0)
{
    assert(clone);
    _allocate();
    Copy(rhs.begin(), begin(), end());
}

template <typename T>
template<typename RHS>
DenseVector<T>::DenseVector(const Vector<RHS> &rhs)
    : _length(rhs.impl().length()), _data(0), _storage(0)
{
    _allocate();
    Copy(rhs.impl().begin(), begin(), end());
}

template <typename T>
DenseVector<T>::DenseVector(size_t length, T *data, void *storage)
    : _length(length), _data(data), _storage(storage)
{
    if (_storage)
        RefCounter.attach(_storage);
}

template <typename T>
DenseVector<T>::~DenseVector()
{
    _release();
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator=(T value)
{
    for (Iterator p = begin(); p != end(); ++p)
        *p = value;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator=(const Range &r)
{
    this->reset(r.length());
    int v = r.beginIndex();
    for (Iterator p = begin(); p != end(); ++p, ++v)
        *p = v;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator=(const DenseVector<T> &v) {
    reset(v.length());
    Copy(v.begin(), begin(), end());
    return *this;
}

template <typename T>
template <typename RHS>
DenseVector<T> &
DenseVector<T>::operator=(const Vector<RHS> &rhs)
{
	reset(rhs.impl().length());
	Copy(rhs.impl().begin(), begin(), end());
    return *this;
}

template <typename T>
template <typename RHS>
DenseVector<T> &
DenseVector<T>::operator+=(const Vector<RHS> &rhs)
{
    assert(length() == rhs.impl().length());
    typename RHS::ConstIterator q = rhs.impl().begin();
    for (Iterator p = begin(); p != end(); ++p, ++q)
        *p += *q;
    return *this;
}

template <typename T>
template <typename RHS>
DenseVector<T> &
DenseVector<T>::operator-=(const Vector<RHS> &rhs)
{
    assert(length() == rhs.impl().length());
    typename RHS::ConstIterator q = rhs.impl().begin();
    for (Iterator p = begin(); p != end(); ++p, ++q)
        *p -= *q;
    return *this;
}

template <typename T>
template <typename RHS>
DenseVector<T> &
DenseVector<T>::operator*=(const Vector<RHS> &rhs)
{
    assert(length() == rhs.impl().length());
    typename RHS::ConstIterator q = rhs.impl().begin();
    for (Iterator p = begin(); p != end(); ++p, ++q)
        *p *= *q;
    return *this;
}

template <typename T>
template <typename RHS>
DenseVector<T> &
DenseVector<T>::operator/=(const Vector<RHS> &rhs)
{
    assert(length() == rhs.impl().length());
    typename RHS::ConstIterator q = rhs.impl().begin();
    for (Iterator p = begin(); p != end(); ++p, ++q)
        *p /= *q;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator+=(T c)
{
    for (Iterator p = begin(); p != end(); ++p)
        *p += c;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator-=(T c)
{
    for (Iterator p = begin(); p != end(); ++p)
        *p -= c;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator*=(T c)
{
    for (Iterator p = begin(); p != end(); ++p)
        *p *= c;
    return *this;
}

template <typename T>
DenseVector<T> &
DenseVector<T>::operator/=(T c)
{
    for (Iterator p = begin(); p != end(); ++p)
        *p /= c;
    return *this;
}

template <typename T>
const T &
DenseVector<T>::operator[](size_t index) const
{
    return _data[index];
}

template <typename T>
T &
DenseVector<T>::operator[](size_t index)
{
    return _data[index];
}

template <typename T>
const DenseVector<T>
DenseVector<T>::operator[](const Range &r) const
{
    return DenseVector(r.length(), _data + r.beginIndex(), _storage);
}

template <typename T>
DenseVector<T>
DenseVector<T>::operator[](const Range &r)
{
    return DenseVector(r.length(), _data + r.beginIndex(), _storage);
}

template <typename T>
template <typename RHS>
const IndirectVectorClosure<DenseVector<T>, typename RHS::Impl>
DenseVector<T>::operator[](const Vector<RHS> &rhs) const
{
    typedef IndirectVectorClosure<DenseVector<T>, typename RHS::Impl> VC;
    return VC(*this, rhs.impl());
}

template <typename T>
void
DenseVector<T>::reset(size_t length)
{
    if (length != _length) {
        assert(_data == _storage);
        _release();
        _length = length;
        _allocate();
    }
}

template <typename T>
void
DenseVector<T>::reset(size_t length, T value)
{
    reset(length);
    std::fill_n(_data, _length, value);
}

template <typename T>
void
DenseVector<T>::resize(size_t length)
{
    if (length != _length) {
        assert(_data == _storage);
        DenseVector<T> v(length);
        Copy(begin(), v.begin(), v.begin() + std::min(length, _length));
        swap(v);
    }
}

template <typename T>
void
DenseVector<T>::resize(size_t length, T value)
{
    if (length != _length) {
        assert(_data == _storage);
        DenseVector<T> v(length);
        Copy(begin(), v.begin(), v.begin() + std::min(length, _length));
        if (length > _length)
            std::fill_n(v.begin() + _length, length - _length, value);
        swap(v);
    }
}

template <typename T>
void
DenseVector<T>::swap(DenseVector<T> &v) {
    std::swap(_length,  v._length);
    std::swap(_data,    v._data);
    std::swap(_storage, v._storage);
}

template <typename T>
void
DenseVector<T>::set(T value) {
    if (value)
        std::fill_n(_data, _length, value);
    else
        memset(_data, 0, _length * sizeof(T));
}

template <typename T>
void
DenseVector<T>::attach(const DenseVector<T> &rhs)
{
    _release();
    _length  = rhs._length;;
    _data    = rhs._data;
    _storage = rhs._storage;
    if (_storage)
        RefCounter.attach(_storage);
}


template <typename T>
void
DenseVector<T>::_allocate()
{
    assert(!_data && !_storage);
    if (length() == 0)
        return;
    _storage = _data = static_cast<T *>(malloc(_length * sizeof(T)));
    assert(_data);
}

template <typename T>
void
DenseVector<T>::_release()
{
    if (_storage) {
         if (RefCounter.detach(_storage)) {
             if (_storage != _data)
                 Logger::Warn(2, "DenseVector: Released by view.\n");
             fflush(stdout);
             free(_storage);
         }
         _storage = NULL;
    }
    _data = NULL;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
std::ostream &
operator<<(std::ostream &o, const DenseVector<T> &x) {
    o.precision(5);
    o.setf(std::ios::fixed);
    o << "[ ";
    for (size_t i = 0; i < x.length(); ++i)
        o << x[i] << " ";
    o << "]";
    return o;
}

template<>
inline std::ostream &
operator<<(std::ostream &o, const DenseVector<unsigned char> &x) {
    o << "[ ";
    for (size_t i = 0; i < x.length(); ++i)
        o << (int)x[i] << " ";
    o << "]";
    return o;
}

template<typename T>
void
WriteVector(FILE *out, const DenseVector<T> &x) {
    WriteUInt64(out, (uint64_t)x.length());
    if (fwrite(x.data(), sizeof(T), x.length(), out) != x.length())
        throw "Write failed.";
    WriteAlignPad(out, x.length() * sizeof(T));
}

template<typename T>
void
ReadVector(FILE *in, DenseVector<T> x) {
    x.reset(ReadUInt64(in));
    if (fread(x.data(), sizeof(T), x.length(), in) != x.length())
        throw "Read failed.";
    ReadAlignPad(in, x.length() * sizeof(T));
}
