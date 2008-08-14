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

#include <algorithm>
#include "util/BitOps.h"
#include "util/FastHash.h"
#include "Types.h"
#include "NgramVector.h"

////////////////////////////////////////////////////////////////////////////////

struct NgramIndexCompare {
    const NgramVector &_vector;
    NgramIndexCompare(const NgramVector &vector) : _vector(vector) { }
    bool operator()(int i, int j) {
        assert((size_t)i < _vector.size() && (size_t)j < _vector.size());
        return (_vector._hists[i] == _vector._hists[j]) ?
            (_vector._words[i] < _vector._words[j]) :
            (_vector._hists[i] < _vector._hists[j]);
    }
};

////////////////////////////////////////////////////////////////////////////////

const NgramIndex NgramVector::Invalid = (NgramIndex)-1;

////////////////////////////////////////////////////////////////////////////////

// Create NgramVector with specified capacity.
NgramVector::NgramVector() : _length(0) {
    _Reindex(1);
}

// Copy constructor.
NgramVector::NgramVector(const NgramVector &v) {
    _length = v._length;
    if (_length != 0) {
        throw "Copying NgramVector";
        _words    = v._words;
        _hists    = v._hists;
        _indices  = v._indices;
        _hashMask = v._hashMask;
    } else
        _Reindex(1);
}

// Return associated index of the value, or -1 if not found.
// In case of collision, apply quadratic probing.
NgramIndex
NgramVector::Find(NgramIndex hist, VocabIndex word) const {
    size_t     skip = 0;
    NgramIndex pos = SuperFastHash(hist, word) & _hashMask;
    NgramIndex index;
    while ((index = _indices[pos]) != Invalid &&
           !(_words[index] == word && _hists[index] == hist))
        pos = (pos + ++skip) & _hashMask;
    return index;
}

// Add value to the hash vector and return the associated index.
// If value already exists, return the existing index.
NgramIndex
NgramVector::Add(NgramIndex hist, VocabIndex word) {
    NgramIndex *pIndex = _FindIndex(hist, word);
    if (*pIndex == Invalid) {
        // Increase index table size as needed.
        if (size() >= _words.length()) {
            Reserve(std::max(1ul<<16, _words.length()*2));  // Double capacity.
            pIndex = _FindIndex(hist, word);  // Update iterator for new index.
        }
        *pIndex = _length;
        _words[_length] = word;
        _hists[_length] = hist;
        _length++;
    }
    return *pIndex;
}

NgramIndex
NgramVector::Add(NgramIndex hist, VocabIndex word, bool *outNew) {
    NgramIndex *pIndex = _FindIndex(hist, word);
    *outNew = (*pIndex == Invalid);
    if (*outNew) {
        // Increase index table size as needed.
        if (size() >= _words.length()) {
            Reserve(std::max(1ul<<16, _words.length()*2));  // Double capacity.
            pIndex = _FindIndex(hist, word);  // Update iterator for new index.
        }
        *pIndex = _length;
        _words[_length] = word;
        _hists[_length] = hist;
        _length++;
    }
    return *pIndex;
}

void
NgramVector::Reserve(size_t capacity) {
    // Reserve index table and value vector with specified capacity.
    if (capacity != _words.length()) {
        _Reindex(nextPowerOf2(capacity + capacity/4));
        _words.resize(capacity);
        _hists.resize(capacity);
    }
}

// Sort elements and return sort index mapping.
void NgramVector::Sort(const VocabVector &vocabMap,
                       const IndexVector &boNgramMap,
                       IndexVector &ngramMap) {
    // Update word and hist indices.
    for (size_t i = 0; i < size(); ++i) {
        _words[i] = vocabMap[_words[i]];
        _hists[i] = boNgramMap[_hists[i]];
    }

    // Sort indices.
    NgramIndexCompare compare(*this);
    IndexVector       sortIndices = Range(0, size());
    std::sort(sortIndices.begin(), sortIndices.end(), compare);

    // Apply ordered indices to values.
    // Build sort mapping that maps old to new indices.
    VocabVector newWords(_words.length());
    IndexVector newHists(_hists.length());
    ngramMap.reset(size());
    for (NgramIndex i = 0; i < (NgramIndex)size(); i++) {
        newWords[i] = _words[sortIndices[i]];
        newHists[i] = _hists[sortIndices[i]];
        ngramMap[sortIndices[i]] = i;
    }
    _words.swap(newWords);
    _hists.swap(newHists);

    // Rebuild index map.
    _Reindex(_indices.length());

    // Build truncated view into words and hists.
    Range r(_length);
    _wordsView.attach(_words[r]);
    _histsView.attach(_hists[r]);
}

void
NgramVector::Serialize(FILE *outFile) const {
    WriteUInt64(outFile, _length);
    WriteVector(outFile, _words);
    WriteVector(outFile, _hists);
    WriteVector(outFile, _indices);
}

void
NgramVector::Deserialize(FILE *inFile) {
    _length = ReadUInt64(inFile);
    ReadVector(inFile, _words);
    ReadVector(inFile, _hists);
    ReadVector(inFile, _indices);
    _hashMask = _indices.length() - 1;
    _wordsView.reset(0);
    _histsView.reset(0);
}

// Return the iterator to the position of the value.
// If value is not found, return the position to insert the value.
// In case of collision, apply quadratic probing.
// NOTE: This function assumes the index table is not full.
NgramIndex *
NgramVector::_FindIndex(NgramIndex hist, VocabIndex word) {
    size_t     skip = 0;
    NgramIndex pos = SuperFastHash(hist, word) & _hashMask;
    NgramIndex index;
    while ((index = _indices[pos]) != Invalid &&
           !(_words[index] == word && _hists[index] == hist))
        pos = (pos + ++skip) & _hashMask;
    return &_indices[pos];
}

// Resize index table to the specified capacity.
void
NgramVector::_Reindex(size_t indexSize) {
    assert(indexSize >= size() && isPowerOf2(indexSize));
    _indices.reset(indexSize, Invalid);
    _hashMask = indexSize - 1;
    for (NgramIndex i = 0; i < (NgramIndex)size(); i++) {
        size_t     skip = 0;
        NgramIndex pos  = SuperFastHash(_hists[i], _words[i]) & _hashMask;
        while (_indices[pos] != Invalid)
            pos = (pos + ++skip) & _hashMask;
        _indices[pos] = i;
    }
}
