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

#ifndef SHAREDPTR_H
#define SHAREDPTR_H

#include "RefCounter.h"

template <typename T>
class SharedPtr {
protected:
    T *_p;

public:
    explicit SharedPtr(T *p = NULL) : _p(p) { }
    SharedPtr(const SharedPtr<T> &p) : _p(p._p)
    { if (_p != NULL) RefCounter.attach(_p); }
    ~SharedPtr() { if (_p != NULL && RefCounter.detach(_p)) delete _p; }

    SharedPtr<T> &operator=(T *p) {
        if (_p != NULL && RefCounter.detach(_p)) delete _p;
        _p = p;
        return *this;
    }
    SharedPtr<T> &operator=(const SharedPtr<T> &p) {
        if (_p != NULL && RefCounter.detach(_p)) delete _p;
        if ((_p = p._p) != NULL) RefCounter.attach(_p);
        return *this;
    }

    T *get()        { return _p; }
    T &operator*()  { return *_p; }
    T *operator->() { return _p; }
    operator T *()  { return _p; }
    const T *get() const        { return _p; }
    const T &operator*() const  { return *_p; }
    const T *operator->() const { return _p; }
    operator const T *() const  { return _p; }
    operator bool() const       { return _p == NULL; }
};

#endif // SHAREDPTR_H
