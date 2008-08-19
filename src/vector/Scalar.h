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

#ifndef SCALAR_H
#define SCALAR_H

#include "Traits.h"

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Scalar {
public:
    class ConstIterator {
    public:
        ConstIterator(T value) : _value(value) { }
        ConstIterator & operator++()           { return *this; }
        T               operator*() const      { return _value; }
        bool operator==(const ConstIterator &iter) const { return true; }
        bool operator!=(const ConstIterator &iter) const { return false; }

    private:
        T _value;
    };

    typedef T         ElementType;
    typedef Scalar<T> Impl;

    Scalar(T value) : _value(value) { }
    const Impl &  impl() const   { return *this; }
    operator      T() const      { return _value; }
    size_t        length() const { return 1; }
    ConstIterator begin() const  { return ConstIterator(_value); }
    ConstIterator end() const    { return ConstIterator(_value); }
    ElementType   operator[](size_t index) const { return _value; }

private:
    T _value;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct Ref<Scalar<T> > {
    typedef Scalar<T> Type;
};


#endif // SCALAR_H
