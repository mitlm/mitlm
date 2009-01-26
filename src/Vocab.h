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

#ifndef VOCAB_H
#define VOCAB_H

#include <string>
#include "util/FastHash.h"
#include "util/ZFile.h"
#include "Types.h"

////////////////////////////////////////////////////////////////////////////////

struct OffsetLen {
    OffsetLen(uint offset = 0, uint len = 0) : Offset(offset), Len(len) { }
    uint Offset;                 // Offset into string buffer
    uint Len;                    // Length of string
};

////////////////////////////////////////////////////////////////////////////////
// Vocab represents a collection of words associated with increasing index.
// Word access by index and index lookup by word can be performed in constant
// time.  To support efficient serialization and memory mapping, we store the
// words in a single string buffer.  Word lengths are stored along with the
// offsets for convenience and efficient comparison/lookup.
//
class Vocab {
protected:
    typedef DenseVector<OffsetLen> OffsetLenVector;

    size_t          _length;
    OffsetLenVector _offsetLens;  // Offsets into string buffer and lengths
    VocabVector     _indices;     // Index table mapping string to index
    std::string     _buffer;      // String buffer storing all words
    size_t          _hashMask;    // Hash mask: hashIndex = hash & hashMask
    bool            _fixedVocab;
    VocabIndex      _unkIndex;

public:
    static const VocabIndex Invalid;          // = (VocabIndex)-1;
    static const VocabIndex EndOfSentence;    // = (VocabIndex)0;

    Vocab(size_t capacity = 1<<16);
    void       SetFixedVocab(bool fixedVocab);
    void       UseUnknown();
    VocabIndex Find(const char *word, size_t len) const;
    VocabIndex Find(const char *word) const { return Find(word, strlen(word)); }
    VocabIndex Add(const char *word, size_t len);
    VocabIndex Add(const char *word) { return Add(word, strlen(word)); }
    void       Reserve(size_t capacity);
    bool       Sort(VocabVector &sortMap);
    void       LoadVocab(ZFile &vocabFile);
    void       SaveVocab(ZFile &vocabFile, bool asBinary=false) const;
    void       Serialize(FILE *outFile) const;
    void       Deserialize(FILE *inFile);

    bool        IsFixedVocab() const        { return _fixedVocab; }
    size_t      size() const                { return _length; }
    size_t      wordlen(VocabIndex n) const { return _offsetLens[n].Len; }
    const char *operator[](VocabIndex n) const
    { return &_buffer[_offsetLens[n].Offset]; }

protected:
    VocabIndex *_FindIndex(const char *word, size_t len);
    void        _Reindex(size_t indexSize);
};

#endif // VOCAB_H
