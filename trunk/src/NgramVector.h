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

#ifndef NGRAMVECTOR_H
#define NGRAMVECTOR_H

#include "Types.h"

namespace mitlm {

////////////////////////////////////////////////////////////////////////////////
// NgramVector represents the n-gram structure within a particular order of the
// n-gram trie.  For each n-gram, it stores the index of the history n-gram in
// the lower-order NgramVector and the index corresponding to the target word.
// The n-grams can be accessed by index.  Lookup of the n-gram index can be
// performed in constant time.
//
class NgramVector {
    friend class NgramModel;
    friend class NgramIndexCompare;

protected:
    size_t              _length;
    VocabVector         _words;
    IndexVector         _hists;
    IndexVector         _indices;   // Index table mapping value to index
    size_t              _hashMask;  // Hash mask: hashIndex = hash & hashMask
    mutable VocabVector _wordsView;
    mutable IndexVector _histsView;

public:
    static const NgramIndex Invalid; // = (NgramIndex)-1;

    NgramVector();
    NgramVector(const NgramVector &v);
    NgramIndex Find(NgramIndex hist, VocabIndex word) const;
    NgramIndex Add(NgramIndex hist, VocabIndex word);
    NgramIndex Add(NgramIndex hist, VocabIndex word, bool *outNew);
    void       Reserve(size_t capacity);
    bool       Sort(const VocabVector &vocabMap, const IndexVector &boNgramMap,
                    IndexVector &ngramMap);
    void       Serialize(FILE *outFile) const;
    void       Deserialize(FILE *inFile);

    size_t             size() const     { return _length; }
    size_t             capacity() const { return _indices.length(); }
    const VocabVector &words() const    { return _wordsView; }
    const IndexVector &hists() const    { return _histsView; }

protected:
    NgramIndex *_FindIndex(NgramIndex hist, VocabIndex word);
    void        _Reindex(size_t indexSize);
};

}

#endif // NGRAMVECTOR_H
