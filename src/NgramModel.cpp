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

#include <stdexcept>
#include <vector>
#include "util/BitOps.h"
#include "util/FastIO.h"
#include "util/Logger.h"
#include "util/ZFile.h"
#include "Types.h"
#include "NgramModel.h"

using std::vector;

#define MAXLINE 4096

////////////////////////////////////////////////////////////////////////////////

NgramModel::NgramModel(size_t order) {
    SetOrder(order);
    _vectors[0].Add(0, 0);
}

void
NgramModel::SetOrder(size_t order) {
    _vectors.resize(order + 1);
    _backoffVectors.resize(order + 1);
}

void
NgramModel::LoadVocab(const ZFile &vocabFile) {
    _vocab.LoadVocab(vocabFile);
    _vocab.SetReadOnly(true);
}

void
NgramModel::SaveVocab(const ZFile &vocabFile, bool asBinary) const {
    _vocab.SaveVocab(vocabFile, asBinary);
}

void
NgramModel::LoadCorpus(vector<CountVector> &countVectors,
                       const ZFile &corpusFile, bool reset) {
    if (corpusFile == NULL) throw std::invalid_argument("Invalid file");

    // Resize vectors and allocate counts.
    countVectors.resize(size());
    countVectors[0].resize(1, 0);
    for (size_t o = 1; o < size(); ++o) {
        size_t capacity = std::max(1ul<<16, nextPowerOf2(_vectors[o].size()));
        if (reset)
            countVectors[o].reset(capacity, 0);
        else
            countVectors[o].resize(capacity, 0);
    }

    // Accumulate counts for each n-gram in corpus file.
    char line[MAXLINE];
    vector<VocabIndex> words(256);
    vector<NgramIndex> hists(size());
    while (getline(corpusFile, line, MAXLINE)) {
        if (strncmp(line, "<DOC ", 5) == 0 || strcmp(line, "</DOC>") == 0)
            continue;

        // Lookup vocabulary indices for each word in the line.
        words.clear();
        words.push_back(Vocab::BeginOfSentence);
        char *p = &line[0];
        while (*p != '\0') {
            while (isspace(*p)) ++p;  // Skip consecutive spaces.
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            size_t len = p - token;
            if (*p != 0) *p++ = 0;
            words.push_back(_vocab.Add(token, len));
        }
        words.push_back(Vocab::EndOfSentence);

        // Add each order n-gram.
        countVectors[0][0] += words.size() - 1;
        for (size_t i = 0; i < words.size(); ++i) {
            VocabIndex word = words[i];
            NgramIndex hist = 0;
            for (size_t j = 1; j < std::min(i + 2, size()); ++j) {
                if (word != Vocab::Invalid) {
                    bool       newNgram;
                    NgramIndex index = _vectors[j].Add(hist, word, &newNgram);
                    if (newNgram && (size_t)index >= countVectors[j].length())
                        countVectors[j].resize(countVectors[j].length() * 2, 0);
                    countVectors[j][index]++;
                    hist     = hists[j];
                    hists[j] = index;
                } else {
                    hist     = hists[j];
                    hists[j] = NgramVector::Invalid;
                }
            }
        }
    }

    // Sort and resize counts to actual size.
    VocabVector vocabMap;
    IndexVector ngramMap(1, 0), boNgramMap;
    _vocab.Sort(vocabMap);
    for (size_t o = 0; o < size(); ++o) {
        boNgramMap.swap(ngramMap);
        _vectors[o].Sort(vocabMap, boNgramMap, ngramMap);
        ApplySort(ngramMap, countVectors[o]);
    }
    _ComputeBackoffs();
}

void
NgramModel::LoadCounts(vector<CountVector> &countVectors,
                       const ZFile &countsFile, bool reset) {
    if (countsFile == NULL) throw std::invalid_argument("Invalid file");

    // Resize vectors and allocate counts.
    countVectors.resize(size());
    countVectors[0].resize(1, 0);
    for (size_t o = 1; o < size(); ++o) {
        size_t capacity = std::max(1ul<<16, nextPowerOf2(_vectors[o].size()));
        if (reset)
            countVectors[o].reset(capacity, 0);
        else
            countVectors[o].resize(capacity, 0);
    }

    // Accumulate counts for each n-gram in counts file.
    char                    line[MAXLINE];
    vector<VocabIndex> words(256);
    while (getline(countsFile, line, MAXLINE)) {
        if (line[0] == '\0' || line[0] == '#') continue;

        words.clear();
        char *p = &line[0];
        while (words.size() < size()) {
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            if (*p == 0) {
                // Last token in line: Add ngram with count
                bool       newNgram;
                size_t     order = words.size();
                NgramIndex index = 0;
                for (size_t i = 1; i < order; ++i)
                    index = _vectors[i].Add(index, words[i - 1]);
                index = _vectors[order].Add(index, words[order - 1], &newNgram);
                if (newNgram && (size_t)index >= countVectors[order].length())
                    countVectors[order].resize(countVectors[order].length() * 2,
                                               0);
                countVectors[order][index] += atoi(token);
                break;  // Move to next line.
            }

            // Not the last token: Lookup word index and add to words.
            size_t len = p - token;
            *p++ = 0;
            if (len > 0) {
                VocabIndex vocabIndex = _vocab.Add(token, len);
                if (vocabIndex == Vocab::Invalid) break;
                words.push_back(vocabIndex);
            }
        }
    }

    // Sort and resize counts to actual size.
    VocabVector vocabMap;
    IndexVector ngramMap(1, 0), boNgramMap;
    _vocab.Sort(vocabMap);
    for (size_t o = 0; o < size(); ++o) {
        boNgramMap.swap(ngramMap);
        _vectors[o].Sort(vocabMap, boNgramMap, ngramMap);
        ApplySort(ngramMap, countVectors[o]);
    }
    _ComputeBackoffs();
}

void
NgramModel::SaveCounts(const vector<CountVector> &countVectors,
                       const ZFile &countsFile) const {
    if (countsFile == NULL) throw std::invalid_argument("Invalid file");

    // Write counts.
    StrVector   ngramWords(size());
    std::string lineBuffer;
    lineBuffer.resize(size() * 32);

    // Write 0th order counts.
    char *ptr = &lineBuffer[0];
    *ptr++ = '\t';
    ptr = CopyUInt(ptr, countVectors[0][0]);
    *ptr++ = '\n';
    *ptr = '\0';
    fputs(&lineBuffer[0], countsFile);

    // Write higher order counts.
    for (size_t o = 1; o < countVectors.size(); ++o) {
        const CountVector &counts = countVectors[o];
        for (NgramIndex i = 0; i < (NgramIndex)countVectors[o].length(); ++i) {
            // Allocate spaces for words, spaces, UInt, \n\0.
            size_t len = GetNgramWords(o, i, ngramWords) + o + 12;
            if (lineBuffer.size() < len)
                lineBuffer.resize(len);
            char *ptr = &lineBuffer[0];
            ptr = CopyString(ptr, ngramWords[0]);
            for (size_t j = 1; j < o; j++) {
                *ptr++ = ' ';
                ptr = CopyString(ptr, ngramWords[j]);
            }
            *ptr++ = '\t';
            ptr = CopyUInt(ptr, counts[i]);
            *ptr++ = '\n';
            *ptr = '\0';
            fputs(&lineBuffer[0], countsFile);
        }
    }
}

void
NgramModel::LoadLM(vector<ProbVector> &probVectors,
                   vector<ProbVector> &bowVectors,
                   const ZFile &lmFile) {
    if (lmFile == NULL) throw std::invalid_argument("Invalid file");

    // Read ARPA LM header.
    char           line[MAXLINE];
    size_t         o, len;
    vector<size_t> ngramLengths(1);
    while (getline(lmFile, line, MAXLINE) && strcmp(line, "\\data\\") != 0)
        /* NOP */;
    while (getline(lmFile, line, MAXLINE)) {
        unsigned int o, len;
        if (sscanf(line, "ngram %u=%u", &o, &len) != 2)
            break;
        assert(o == ngramLengths.size());
        ngramLengths.push_back(len);
    }

    // Allocate buffers and read counts.
    _vocab.Reserve(ngramLengths[1]);
    probVectors.resize(size());
    probVectors[0].resize(1, 0.0);
    bowVectors.resize(size() - 1);
    bowVectors[0].resize(1, 0.0);
    for (o = 1; o < size(); o++) {
        ProbVector &probs  = probVectors[o];
        ProbVector &bows   = bowVectors[o];
        bool        hasBow = (o < size() - 1);

        // Preallocate buffer for n-grams.
        _vectors[o].Reserve(ngramLengths[o]);
        probs.reset(ngramLengths[o]);
        if (hasBow) bows.reset(ngramLengths[o]);

        getline(lmFile, line, MAXLINE);
        unsigned int i;
        if (sscanf(line, "\\%u-ngrams:", &i) != 1 || i != o) {
            throw std::invalid_argument("Unexpected file format.");
        }
        while (true) {
            getline(lmFile, line, MAXLINE);
            size_t lineLen = strlen(line);
            if (line[0] == '\0') break;  // Empty line ends section.
            char *p = &line[0];

            // Read log probability.
            Prob prob = (Prob)pow(10.0, strtod(p, &p)); p++;

            // Read i words.
            NgramIndex index = 0;
            for (i = 1; i <= o; ++i) {
                const char *token = p;
                while (*p != 0 && !isspace(*p)) ++p;
                len = p - token;
                *p++ = 0;
                VocabIndex vocabIndex = _vocab.Add(token, len);
                if (vocabIndex == Vocab::Invalid) {
                    index = NgramVector::Invalid;
                    break;
                }
                index = _vectors[i].Add(index, vocabIndex);
            }
            if (index == NgramVector::Invalid) break;

            // Set probability and backoff weight.
            probs[index] = prob;
            if (hasBow) {
                // Read optional backoff weight.
                bows[index] = (p >= &line[lineLen]) ?
                    NAN : (Prob)pow(10.0, strtod(p, &p));
            }
        }
    }

    // Read ARPA LM footer.
    while (getline(lmFile, line, MAXLINE) &&
           strcmp(line, "\\end\\") != 0)  /* NOP */;

    // Sort and resize probs/bows to actual size.
    VocabVector vocabMap;
    IndexVector ngramMap(1, 0), boNgramMap;
    _vocab.Sort(vocabMap);
    for (size_t o = 0; o < size(); ++o) {
        boNgramMap.swap(ngramMap);
        _vectors[o].Sort(vocabMap, boNgramMap, ngramMap);
        ApplySort(ngramMap, probVectors[o]);
        if (o < bowVectors.size())
            ApplySort(ngramMap, bowVectors[o]);
    }
    _ComputeBackoffs();
}

void
NgramModel::SaveLM(const vector<ProbVector> &probVectors,
                   const vector<ProbVector> &bowVectors,
                   const ZFile &lmFile) const {
    if (lmFile == NULL) throw std::invalid_argument("Invalid file");

    // Write ARPA backoff LM header.
    fputs("\n\\data\\\n", lmFile);
    for (size_t o = 1; o < size(); o++)
        fprintf(lmFile, "ngram %lu=%lu\n",
                (unsigned long)o, (unsigned long)_vectors[o].size());

    // Write lower order n-grams with probabilities and backoff weights.
    StrVector   ngramWords(size() - 1);
    std::string lineBuffer;
    lineBuffer.resize(size() * 32); // 32 chars/word.
    for (size_t o = 1; o < size() - 1; o++) {
        fprintf(lmFile, "\n\\%lu-grams:\n", (unsigned long)o);
        const ProbVector &probs = probVectors[o];
        const ProbVector &bows  = bowVectors[o];
        assert(probs.length() == _vectors[o].size());
        assert(bows.length() == _vectors[o].size());
        for (NgramIndex i = 0; i < (NgramIndex)_vectors[o].size(); ++i) {
            // Allocate spaces for Prob, words, spaces, Prob, \n\0.
            size_t len = GetNgramWords(o, i, ngramWords) + 22;
            if (lineBuffer.size() < len)
                lineBuffer.resize(len);
            char *ptr = &lineBuffer[0];
            ptr = CopyLProb(ptr, probs[i]);
            *ptr++ = '\t';
            ptr = CopyString(ptr, ngramWords[0]);
            for (size_t j = 1; j < o; ++j) {
                *ptr++ = ' ';
                ptr = CopyString(ptr, ngramWords[j]);
            }
            if (!isnan(bows[i])) {
                *ptr++ = '\t';
                ptr = CopyLProb(ptr, bows[i]);
            }
            *ptr++ = '\n';
            *ptr = '\0';
            assert((size_t)(ptr - lineBuffer.data()) < lineBuffer.size());
            fputs(&lineBuffer[0], lmFile);
        }
    }

    // Write highest order n-grams without backoff weights.
    {
        size_t o = size() - 1;
        fprintf(lmFile, "\n\\%lu-grams:\n", (unsigned long)o);
        const ProbVector &probs = probVectors[o];
        for (NgramIndex i = 0; i < (NgramIndex)_vectors[o].size(); ++i) {
            // Allocate spaces for Prob, words, spaces, \n\0.
            size_t len = GetNgramWords(o, i, ngramWords) + 12;
            if (lineBuffer.size() < len)
                lineBuffer.resize(len);
            char *ptr = &lineBuffer[0];
            ptr = CopyLProb(ptr, probs[i]);
            *ptr++ = '\t';
            ptr = CopyString(ptr, ngramWords[0]);
            for (size_t j = 1; j < o; ++j) {
                *ptr++ = ' ';
                ptr = CopyString(ptr, ngramWords[j]);
            }
            *ptr++ = '\n';
            *ptr = '\0';
            fputs(&lineBuffer[0], lmFile);
        }
    }

    // Write ARPA backoff LM footer.
    fputs("\n\\end\\\n", lmFile);
}

void
NgramModel::LoadEvalCorpus(vector<CountVector> &probCountVectors,
                           vector<CountVector> &bowCountVectors,
                           BitVector &vocabMask,
                           const ZFile &corpusFile,
                           size_t &outNumOOV,
                           size_t &outNumWords) const {
    if (corpusFile == NULL) throw std::invalid_argument("Invalid file");

    // Allocate count vectors.
    probCountVectors.resize(size());
    bowCountVectors.resize(size() - 1);
    for (size_t i = 0; i < size(); i++)
        probCountVectors[i].reset(_vectors[i].size(), 0);
    for (size_t i = 0; i < size() - 1; i++)
        bowCountVectors[i].reset(_vectors[i].size(), 0);

    // Accumulate counts of prob/bow for computing perplexity of corpusFilename.
    char                    line[MAXLINE];
    size_t                  numOOV = 0;
    size_t                  numWords = 0;
    vector<VocabIndex> words(256);
    while (getline(corpusFile, line, MAXLINE)) {
        if (strncmp(line, "<DOC ", 5) == 0 || strcmp(line, "</DOC>") == 0)
            continue;

        // Lookup vocabulary indices for each word in the line.
        words.clear();
        words.push_back(Vocab::BeginOfSentence);
        char *p = &line[0];
        while (*p != 0) {
            while (isspace(*p)) ++p;  // Skip consecutive spaces.
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            size_t len = p - token;
            if (*p != 0) *p++ = 0;
            words.push_back(_vocab.Find(token, len));
        }
        words.push_back(Vocab::EndOfSentence);

        // Add each top order n-gram.
        size_t ngramOrder = 2;
        for (size_t i = 1; i < words.size(); i++) {
            if (words[i] == Vocab::Invalid || !vocabMask[words[i]]) {
                // OOV word encountered.  Reset order to unigrams.
                ngramOrder = 1;
                numOOV++;
            } else {
                NgramIndex index;
                size_t     boOrder = ngramOrder;
                while ((index = _Find(&words[i-boOrder+1], boOrder)) == -1) {
                    --boOrder;
                    NgramIndex hist = _Find(&words[i - boOrder], boOrder);
                    if (hist != (NgramIndex)-1)
                        bowCountVectors[boOrder][hist]++;
                }
                ngramOrder = std::min(ngramOrder + 1, size() - 1);
                probCountVectors[boOrder][index]++;
                numWords++;
            }
        }
    }
    outNumOOV   = numOOV;
    outNumWords = numWords;
}

void
NgramModel::LoadFeatures(vector<DoubleVector> &featureVectors,
                         const ZFile &featureFile, size_t maxSize) const {
    if (featureFile == NULL) throw std::invalid_argument("Invalid file");

    // Allocate space for feature vectors.
    if (maxSize == 0 || maxSize > size())
        maxSize = size();
    featureVectors.resize(maxSize);
    for (size_t i = 0; i < maxSize; i++)
        featureVectors[i].reset(sizes(i), 0);  // Initialize to 0.

    // Load feature value for each n-gram in feature file.
    char                    line[MAXLINE];
    vector<VocabIndex> words(256);
    while (getline(featureFile, line, MAXLINE)) {
        if (line[0] == '\0' || line[0] == '#') continue;
        words.clear();
        char *p = &line[0];
        while (true) {
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            if (*p == 0)
            {
                // Last token in line: Set feature value.
                NgramIndex index = 0;
                for (size_t i = 1; i <= words.size(); i++)
                    index = _vectors[i].Find(index, words[i - 1]);
                if (index == -1)
                    Logger::Warn(1, "Feature skipped.\n");
                else
                    featureVectors[words.size()][index] = atof(token);
                break;  // Move to next line.
            }

            // Not the last token: Lookup word index and add to words.
            size_t len = p - token;
            *p++ = 0;
            if (len > 0) words.push_back(_vocab.Find(token, len));
            if (words.size() >= maxSize) break;
        }
    }
}

void
NgramModel::LoadComputedFeatures(vector<DoubleVector> &featureVectors,
                                 const char *featureFile,
                                 size_t maxSize) const {
    std::string strFilename(featureFile);
    size_t colonIndex = strFilename.find(':');
    const char *featFunc, *filename;
    if (colonIndex == std::string::npos) {
        featFunc = NULL;
        filename = strFilename.c_str();
    } else {
        strFilename[colonIndex] = 0;
        featFunc = strFilename.c_str();
        filename = &strFilename[colonIndex + 1];
    }

    ZFile f(filename, "r");
    if (featFunc == NULL)
        LoadFeatures(featureVectors, f, maxSize);
    else if (strcmp(featFunc, "freq") == 0)
        _LoadFrequency(featureVectors, f, maxSize);
    else if (strcmp(featFunc, "entropy") == 0)
        _LoadEntropy(featureVectors, f, maxSize);
    else {
        LoadFeatures(featureVectors, f, maxSize);
        if (strcmp(featFunc, "log") == 0)
            for (size_t o = 0; o < featureVectors.size(); ++o)
                featureVectors[o] = log(featureVectors[o] + 1e-99);
        else if (strcmp(featFunc, "log1p") == 0)
            for (size_t o = 0; o < featureVectors.size(); ++o)
                featureVectors[o] = log(featureVectors[o] + 1);
        else if (strcmp(featFunc, "pow2") == 0)
            for (size_t o = 0; o < featureVectors.size(); ++o)
                featureVectors[o] *= featureVectors[o];
        else if (strcmp(featFunc, "pow3") == 0)
            for (size_t o = 0; o < featureVectors.size(); ++o)
                featureVectors[o] = pow(featureVectors[o], 3);
        else {
            Logger::Error(1, "Unknown feature function: %s\n", featFunc);
            exit(1);
        }
    }

    for (size_t o = 0; o < featureVectors.size(); ++o) {
        if (anyTrue(featureVectors[o] > 20.0)) {
            Logger::Warn(1, "Feature value %s exceed 20.0.\n", featureFile);
            break;
        }
    }
}

size_t
NgramModel::GetNgramWords(size_t order,
                          NgramIndex index,
                          StrVector &words) const {
    // TODO: If n-grams are sorted, we can cache the history for each order
    //       and short circuit the iteration once the history matches.
    //       Primarily useful for higher orders.
    size_t totalLength = 0;
    for (size_t i = order; i > 0; --i) {
        const NgramVector &v(_vectors[i]);
        assert(index >= 0 && index < (NgramIndex)v.size());
        VocabIndex word = v._words[index];
        words[i - 1]    = _vocab[word];
        totalLength    += _vocab.wordlen(word);
        index           = v._hists[index];
    }
    return totalLength;
}

// Add all n-grams in m to current model.  Call FinalizeModel() afterwards to
// sort n-grams and compute backoffs.
void
NgramModel::ExtendModel(const NgramModel &m,
                        VocabVector &vocabMap,
                        vector<IndexVector> &ngramMap) {
    // Map vocabulary.
    vocabMap.reset(m._vocab.size());
    for (size_t i = 0; i < m._vocab.size(); ++i)
        vocabMap[i] = _vocab.Add(m._vocab[i]);

    // Map n-grams.
    if (size() == 0) {
        // Current model is empty.  Make a copy.
        _vectors = m._vectors;
        ngramMap.resize(size());
        for (size_t o = 0; o < size(); ++o)
            ngramMap[o] = Range(m.sizes(o));
    } else {
        // Merge m into current model.
        if (size() < m.size())
            _vectors.resize(m.size());
        ngramMap.resize(size());
        ngramMap[0].reset(1, 0);
        for (size_t o = 1; o < m.size(); ++o) {
            const VocabVector &words(m.words(o));
            const IndexVector &hists(m.hists(o));
            ngramMap[o].reset(words.length());
            for (size_t i = 0; i < words.length(); ++i) {
                NgramIndex hist = ngramMap[o-1][hists[i]];
                VocabIndex word = vocabMap[words[i]];
                ngramMap[o][i] = _vectors[o].Add(hist, word);
            }
        }
    }
}

void
NgramModel::SortModel(VocabVector &vocabMap,
                      vector<IndexVector> &ngramMap) {
    // Sort and resize counts to actual size.
    _vocab.Sort(vocabMap);
    ngramMap.resize(size());
    ngramMap[0].reset(1, 0);
    for (size_t o = 1; o < size(); ++o)
        _vectors[o].Sort(vocabMap, ngramMap[o-1], ngramMap[o]);
    _ComputeBackoffs();
}

void
NgramModel::Serialize(FILE *outFile) const {
    WriteHeader(outFile, "NgramModel");
    _vocab.Serialize(outFile);
    WriteUInt64(outFile, size());
    for (unsigned int i = 0; i < size(); i++)
        _vectors[i].Serialize(outFile);
}

void
NgramModel::Deserialize(FILE *inFile) {
    VerifyHeader(inFile, "NgramModel");
    _vocab.Deserialize(inFile);
    _vectors.resize(ReadUInt64(inFile));
    for (unsigned int i = 0; i < size(); i++)
        _vectors[i].Deserialize(inFile);
}

template <class T>
void
NgramModel::ApplySort(const IndexVector &ngramMap,
                     DenseVector<T> &data,
                     size_t length,
                     T defValue) {
    assert(data.length() >= ngramMap.length());
    if (length == 0) length = ngramMap.length();
    DenseVector<T> sortedData(length, defValue);
    for (size_t i = 0; i < ngramMap.length(); ++i)
        sortedData[ngramMap[i]] = data[i];
    data.swap(sortedData);
}

NgramIndex
NgramModel::_Find(const VocabIndex *words, size_t wordsLen) const {
    NgramIndex index = 0;
    for (size_t i = 0; i < wordsLen; ++i)
        index = _vectors[i+1].Find(index, words[i]);
    return index;
}

void
NgramModel::_ComputeBackoffs() {
    // Assign unique backoff of 0 to unigrams.
    _backoffVectors[0].resize(_vectors[0].size(), 0);
    if (size() > 1)
        _backoffVectors[1].resize(_vectors[1].size(), 0);

    // Compute backoffs for bigram via vocabulary lookup.
    if (size() > 2) {
        IndexVector &backoffs(_backoffVectors[2]);
        size_t       oldSize = backoffs.length();
        backoffs.resize(_vectors[2].size());
        for (NgramIndex i = oldSize; i < (NgramIndex)backoffs.length(); ++i)
            backoffs[i] = _vectors[1].Find(0, _vectors[2]._words[i]);
    }

    // For higher order n-grams, need to recursively lookup history index.
    VocabVector ngramVocabs(size());
    for (size_t o = 3; o < size(); o++) {
        NgramVector &v(_vectors[o]);
        IndexVector &backoffs(_backoffVectors[o]);
        size_t       oldSize  = backoffs.length();
        NgramIndex   prevHist = (NgramIndex)-1;
        backoffs.resize(v.size());
        for (NgramIndex i = oldSize; i < (NgramIndex)backoffs.length(); ++i) {
            if (v._hists[i] != prevHist) {
                NgramIndex index = prevHist = v._hists[i];
                size_t     j     = o;
                while (--j > 1) {
                    ngramVocabs[j] = _vectors[j]._words[index];
                    index          = _vectors[j]._hists[index];
                }
            }
            ngramVocabs[o] = v._words[i];
            backoffs[i]    = _Find(&ngramVocabs[2], o - 1);
        }
    }
}

void
NgramModel::_LoadFrequency(vector<DoubleVector> &freqVectors,
                              const ZFile &corpusFile, size_t maxSize) const {
    if (corpusFile == NULL) throw std::invalid_argument("Invalid file");

    // Resize vectors and allocate counts.
    if (maxSize == 0 || maxSize > size())
        maxSize = size();
    int numDocs = 0;
    vector<CountVector> countVectors(maxSize);
    freqVectors.resize(maxSize);
    for (size_t o = 0; o < maxSize; ++o) {
        countVectors[o].resize(sizes(o), 0);
        freqVectors[o].resize(sizes(o), 0);
    }

    // Accumulate counts for each n-gram in corpus file.
    char line[MAXLINE];
    vector<VocabIndex> words(256);
    vector<NgramIndex> hists(maxSize);
    while (getline(corpusFile, line, MAXLINE)) {
        if (strcmp(line, "</DOC>") == 0) {
            // Accumulate frequency.
            numDocs++;
            for (size_t o = 1; o < countVectors.size(); ++o) {
                for (size_t i = 0; i < countVectors[o].length(); ++i) {
                    if (countVectors[o][i] > 0) {
                        freqVectors[o][i] += 1;
                        countVectors[o][i] = 0;
                    }
                }
            }
            continue;
        } else if (strncmp(line, "<DOC ", 5) == 0)
            continue;

        // Lookup vocabulary indices for each word in the line.
        words.clear();
        words.push_back(Vocab::BeginOfSentence);
        char *p = &line[0];
        while (*p != '\0') {
            while (isspace(*p)) ++p;  // Skip consecutive spaces.
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            size_t len = p - token;
            if (*p != 0) *p++ = 0;
            words.push_back(_vocab.Find(token, len));
        }
        words.push_back(Vocab::EndOfSentence);

        // Add each order n-gram.
        countVectors[0][0] += words.size() - 1;
        for (size_t i = 0; i < words.size(); ++i) {
            VocabIndex word = words[i];
            NgramIndex hist = 0;
            for (size_t j = 1; j < std::min(i + 2, maxSize); ++j) {
                if (word != Vocab::Invalid) {
                    NgramIndex index = _vectors[j].Find(hist, word);
                    if (index >= 0)
                        countVectors[j][index]++;
                    else
                        Logger::Warn(1, "DocFreq feature skipped.\n");
                    hist     = hists[j];
                    hists[j] = index;
                } else {
                    hist     = hists[j];
                    hists[j] = NgramVector::Invalid;
                }
            }
        }
    }

    // Finalize frequency computation.
    for (size_t o = 1; o < maxSize; o++)
        freqVectors[o] /= numDocs;
}

void
NgramModel::_LoadEntropy(vector<DoubleVector> &entropyVectors,
                            const ZFile &corpusFile, size_t maxSize) const {
    if (corpusFile == NULL) throw std::invalid_argument("Invalid file");

    // Resize vectors and allocate counts.
    if (maxSize == 0 || maxSize > size())
        maxSize = size();
    int numDocs = 0;
    vector<CountVector> countVectors(maxSize);
    vector<CountVector> totCountVectors(maxSize);
    entropyVectors.resize(maxSize);
    for (size_t o = 0; o < maxSize; ++o) {
        countVectors[o].resize(sizes(o), 0);
        totCountVectors[o].resize(sizes(o), 0);
        entropyVectors[o].resize(sizes(o), 0);
    }

    // Accumulate counts for each n-gram in corpus file.
    char line[MAXLINE];
    vector<VocabIndex> words(256);
    vector<NgramIndex> hists(maxSize);
    while (getline(corpusFile, line, MAXLINE)) {
        if (strcmp(line, "</DOC>") == 0) {
            // Accumulate frequency.
            numDocs++;
            for (size_t o = 1; o < countVectors.size(); ++o) {
                for (size_t i = 0; i < countVectors[o].length(); ++i) {
                    int c = countVectors[o][i];
                    if (c > 0) {
                        totCountVectors[o][i] += c;
                        entropyVectors[o][i] += c * log(c);
                        countVectors[o][i] = 0;
                    }
                }
            }
            continue;
        } else if (strncmp(line, "<DOC ", 5) == 0)
            continue;

        // Lookup vocabulary indices for each word in the line.
        words.clear();
        words.push_back(Vocab::BeginOfSentence);
        char *p = &line[0];
        while (*p != '\0') {
            while (isspace(*p)) ++p;  // Skip consecutive spaces.
            const char *token = p;
            while (*p != 0 && !isspace(*p))  ++p;
            size_t len = p - token;
            if (*p != 0) *p++ = 0;
            words.push_back(_vocab.Find(token, len));
        }
        words.push_back(Vocab::EndOfSentence);

        // Add each order n-gram.
        countVectors[0][0] += words.size() - 1;
        for (size_t i = 0; i < words.size(); ++i) {
            VocabIndex word = words[i];
            NgramIndex hist = 0;
            for (size_t j = 1; j < std::min(i + 2, maxSize); ++j) {
                if (word != Vocab::Invalid) {
                    NgramIndex index = _vectors[j].Find(hist, word);
                    if (index >= 0)
                        countVectors[j][index]++;
                    else
                        Logger::Warn(1, "DocFreq feature skipped.\n");
                    hist     = hists[j];
                    hists[j] = index;
                } else {
                    hist     = hists[j];
                    hists[j] = NgramVector::Invalid;
                }
            }
        }
    }

    // Finalize entropy computation.
    double invLogNumDocs = 1.0 / log((double)numDocs);
    for (size_t o = 1; o < maxSize; o++)
        entropyVectors[o] = CondExpr(
            totCountVectors[o] == 0, 0.0,
            ((entropyVectors[o] / -totCountVectors[o])
             + log(asDouble(totCountVectors[o]))) * invLogNumDocs);
}
