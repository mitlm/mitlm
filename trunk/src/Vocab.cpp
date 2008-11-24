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
#include <fstream>
#include <iterator>
#include <stdexcept>
#include "util/FastIO.h"
#include "Vocab.h"

////////////////////////////////////////////////////////////////////////////////

struct VocabIndexCompare
{
    const Vocab &_vocab;
    VocabIndexCompare(const Vocab &vocab) : _vocab(vocab) { }
    bool operator()(int i, int j) { return strcmp(_vocab[i], _vocab[j]) < 0; }
};

////////////////////////////////////////////////////////////////////////////////

const VocabIndex Vocab::Invalid         = (VocabIndex)-1;
const VocabIndex Vocab::EndOfSentence   = (VocabIndex)0;
const VocabIndex Vocab::BeginOfSentence = (VocabIndex)1;

////////////////////////////////////////////////////////////////////////////////

// Create Vocab with specified capacity.
Vocab::Vocab(size_t capacity) : _length(0), _readOnly(false) {
    Reserve(capacity);
    Add("</s>");
    Add("<s>");
}

void
Vocab::SetReadOnly(bool readOnly) {
    _readOnly = readOnly;
}

// Return associated index of the word, or Invalid if not found.
// In case of collision, apply quadratic probing.
VocabIndex
Vocab::Find(const char *word, size_t len) const {
    size_t     skip = 0;
    VocabIndex pos  = StringHash(word, len) & _hashMask;
    VocabIndex index;
    while ((index = _indices[pos]) != Invalid &&
           !(wordlen(index)==len && strncmp(operator[](index), word, len)==0)) {
        pos = (pos + ++skip) & _hashMask;
    }
    return index;
}

// Add word to the vocab and return the associated index.
// If word already exists, return the existing index.
VocabIndex
Vocab::Add(const char *word, size_t len) {
    VocabIndex *pIndex = _FindIndex(word, len);
    if (*pIndex == Invalid && !_readOnly) {
        // Increase index table size as needed.
        if (size() >= _offsetLens.length()) {
            Reserve(std::max((size_t)1<<16, _offsetLens.length()*2));
            pIndex = _FindIndex(word, len);
        }
        *pIndex = _length;
        _offsetLens[_length++] = OffsetLen(_buffer.size(), len);
        _buffer.append(word, len + 1);        // Include terminating NULL.
    }
    return *pIndex;
}

void
Vocab::Reserve(size_t capacity) {
    // Reserve index table and value vector with specified capacity.
    if (capacity != _offsetLens.length()) {
        _Reindex(nextPowerOf2(capacity + capacity/4));
        _offsetLens.resize(capacity);
    }
}

// Sort the vocabulary and output the mapping from original to new index.
void
Vocab::Sort(VocabVector &sortMap) {
    // Sort indices using vocab index comparison function.
    // - Skip the first two words: </s> and <s>
    VocabIndexCompare compare(*this);
    VocabVector       sortIndices = Range(size());
    std::sort(sortIndices.begin() + 2, sortIndices.end(), compare);

    // Build new string buffer for the sorted words.
    // Change offsets to refer to new string buffer.
    // Build sort mapping that maps old to new indices.
    std::string     newBuffer;
    OffsetLenVector newOffsetLens(size());
    newBuffer.reserve(_buffer.size());
    sortMap.reset(size());
    for (VocabIndex i = 0; i < (VocabIndex)size(); ++i) {
        const OffsetLen &offsetLen = _offsetLens[sortIndices[i]];
        newOffsetLens[i] = OffsetLen(newBuffer.length(), offsetLen.Len);
        newBuffer.append(&_buffer[offsetLen.Offset], offsetLen.Len + 1);
        sortMap[sortIndices[i]] = i;
    }
    _buffer.swap(newBuffer);
    _offsetLens.swap(newOffsetLens);

    // Rebuild index map by applying sortMap.
    MaskAssign(_indices != Invalid, sortMap[_indices], _indices);
}

////////////////////////////////////////////////////////////////////////////////

// Loads vocabulary from file where each word appears on a non-# line.
void
Vocab::LoadVocab(const ZFile &vocabFile) {
    if (ReadUInt64(vocabFile) == MITLMv1) {
        Deserialize(vocabFile);
    } else {
        fseek(vocabFile, 0, SEEK_SET);
        char   line[4096];
        size_t len = 0;
        while (!feof(vocabFile)) {
            getline(vocabFile, line, 4096, &len);
            if (len > 0 && line[0] != '#')
                Add(line, len);
        }
    }
}

// Saves vocabulary to file with each word on its own line.
void
Vocab::SaveVocab(const ZFile &vocabFile, bool asBinary) const {
    if (asBinary) {
        WriteUInt64(vocabFile, MITLMv1);
        Serialize(vocabFile);
    } else {
        for (const OffsetLen *p = _offsetLens.begin();
             p !=_offsetLens.begin() + _length; ++p) {
            fputs(&_buffer[p->Offset], vocabFile);
            fputc('\n', vocabFile);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void
Vocab::Serialize(FILE *outFile) const {
    WriteHeader(outFile, "Vocab");
    WriteVector(outFile, _offsetLens);
    WriteVector(outFile, _indices);
    WriteString(outFile, _buffer);
}

void
Vocab::Deserialize(FILE *inFile) {
    VerifyHeader(inFile, "Vocab");
    ReadVector(inFile, _offsetLens);
    ReadVector(inFile, _indices);
    ReadString(inFile, _buffer);
    _hashMask = _indices.length() - 1;
}

////////////////////////////////////////////////////////////////////////////////

// Return the iterator to the position of the word.
// If word is not found, return the position to insert the word.
// In case of collision, apply quadratic probing.
// NOTE: This function assumes the index table is not full.
VocabIndex *
Vocab::_FindIndex(const char *word, size_t len) {
    size_t     skip = 0;
    VocabIndex pos  = StringHash(word, len) & _hashMask;
    VocabIndex index;
    while ((index = _indices[pos]) != Invalid &&
           !(wordlen(index)==len && strncmp(operator[](index), word, len)==0)) {
        pos = (pos + ++skip) & _hashMask;
    }
    return &_indices[pos];
}

// Resize index table to the specified capacity.
void
Vocab::_Reindex(size_t indexSize) {
    assert(indexSize > size() && isPowerOf2(indexSize));

    _indices.reset(indexSize, Invalid);
    _hashMask = indexSize - 1;
    OffsetLen *p = _offsetLens.begin();
    for (VocabIndex i = 0; i < (VocabIndex)size(); ++i, ++p) {
        size_t     skip = 0;
        VocabIndex pos  = StringHash(&_buffer[p->Offset], p->Len) & _hashMask;
        while (_indices[pos] != Invalid)
            pos = (pos + ++skip) & _hashMask;
        _indices[pos] = i;
    }
}
