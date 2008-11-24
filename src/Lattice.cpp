//////////////////////////////////////////////////////////////////////////
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

#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <tr1/unordered_map>
#include "util/FastIO.h"
#include "Lattice.h"

using std::vector;
using std::tr1::unordered_map;

#define MAXLINE 1024

////////////////////////////////////////////////////////////////////////////////

struct ArcCompare
{
    const Lattice &_lattice;
    ArcCompare(const Lattice &lattice) : _lattice(lattice) { }
    bool operator()(uint i, uint j) const {
        return (_lattice.arcStarts()[i] == _lattice.arcStarts()[j]) ?
            (_lattice.arcEnds()[i] < _lattice.arcEnds()[j]) :
            (_lattice.arcStarts()[i] < _lattice.arcStarts()[j]);
    }
};

struct Path {
    Path(float s, NodeIndex n) : score(s), node(n) { }
    bool operator<(const Path &x) const { return score > x.score; }
    float     score;
    NodeIndex node;
};

struct OraclePath {
    OraclePath(int w, float s, uint a, ushort prevR, ushort r)
      : wer(w), score(s), arcIndex(a), prevRefIndex(prevR), refIndex(r) { }
    bool operator<(const OraclePath &x) const
      { return (wer == x.wer) ? score > x.score : wer > x.wer; }
    int    wer;
    float  score;
    uint   arcIndex;
    ushort prevRefIndex;
    ushort refIndex;
};

typedef std::pair<NodeIndex, uint> Backtrace;
struct BacktraceHash {
    size_t operator()(const Backtrace &b) const
    { return SuperFastHash(b.first, b.second); }
};

typedef std::pair<NodeIndex, NgramIndex> NodeNgram;
struct NodeNgramHash {
    size_t operator()(const NodeNgram &b) const
    { return SuperFastHash(b.first, b.second); }
};
typedef unordered_map<NodeNgram, NodeIndex, NodeNgramHash> NodeNgramMap;

////////////////////////////////////////////////////////////////////////////////

void
Lattice::LoadLattice(const ZFile &latticeFile) {
    // TODO: Support optional weights.
    if (latticeFile == NULL) throw std::invalid_argument("Invalid file");

    NodeIndex  startNode, endNode;
    char       wordStr[MAXLINE], line[MAXLINE];
    float      weight;
    VocabIndex word;
    size_t     numArcs = 0;

    // Read lattice file.
    _Reserve(1024);
    getline(latticeFile, line, MAXLINE);
    if (strcmp(line, "#FSTBasic MinPlus") != 0)
        throw std::runtime_error("Invalid lattice FST header.");
    getline(latticeFile, line, MAXLINE);
    if (strcmp(line, "I 0") != 0)
        throw std::runtime_error("Invalid lattice FST initial state.");
    while (getline(latticeFile, line, MAXLINE)) {
        if (line[0] == 'T') {
            weight = 0;
            if (sscanf(line, "T %i %i %s %s %f",
                       &startNode, &endNode, wordStr, wordStr, &weight) < 4)
                throw std::runtime_error("Invalid lattice FST transition.");
            if (startNode >= endNode)
                throw std::runtime_error("FST is not topologically sorted.");
            word = _lm.vocab().Find(wordStr);
            if (word == Vocab::Invalid)
                throw std::runtime_error("FST contains OOV word.");
        } else if (line[0] == 'F') {
            weight = 0;
            if (sscanf(line, "F %i %f", &startNode, &weight) < 1)
                throw std::runtime_error("Invalid lattice FST final state.");
            // Add </s> from final states to unique _finalNode.
            endNode = std::numeric_limits<NodeIndex>::max();
            word    = Vocab::EndOfSentence;
        } else if (line[0] == 'P') {
            continue;     // Ignore state potentials.
        } else
            throw std::runtime_error("Invalid lattice FST entry.");

        if (numArcs >= _arcStarts.length())
            _Reserve(_arcStarts.length() * 2);
        _arcStarts[numArcs]      = startNode;
        _arcEnds[numArcs]        = endNode;
        _arcWords[numArcs]       = word;
        _arcBaseWeights[numArcs] = weight;
        ++numArcs;
    }

    // Sort transitions by arcStart, arcEnd.
    _Sort(numArcs, ArcCompare(*this));

    // Compute first index of each start node and update final node index.
    _finalNode = _arcStarts[_arcStarts.length() - 1] + 1;
    _nodeArcs.reset(_finalNode + 2);
    NodeIndex node = (NodeIndex)-1;
    for (uint i = 0; i < numArcs; ++i) {
        if (_arcStarts[i] != node) {
            node = _arcStarts[i];
            assert(node < _finalNode);
            _nodeArcs[node] = i;
        }
        if (_arcEnds[i] == std::numeric_limits<NodeIndex>::max())
            _arcEnds[i] = _finalNode;
    }
    _nodeArcs[_finalNode]   = _arcStarts.length();
    _nodeArcs[_finalNode+1] = _arcStarts.length();

    // Compute arc -> ngram prob/bow index mapping.
    _ComputeArcNgramMapping();

    UpdateWeights();
}

void
Lattice::SaveLattice(const ZFile &latticeFile) const {
    if (latticeFile == NULL) throw std::invalid_argument("Invalid file");

    fprintf(latticeFile, "#FSTBasic MinPlus\n");
    fprintf(latticeFile, "I 0\n");
    for (NodeIndex i = 0; i < _arcEnds.length(); ++i) {
        if (_arcEnds[i] == _finalNode) {
            assert(_arcWords[i] == Vocab::EndOfSentence);
            fprintf(latticeFile, "F %u %.5f\n", _arcStarts[i], _arcWeights[i]);
        } else {
            const char *word = _lm.vocab()[_arcWords[i]];
            if (fabs(_arcWeights[i]) < 0.001)
                fprintf(latticeFile, "T %u %u %s %s\n",
                        _arcStarts[i], _arcEnds[i], word, word);
            else
                fprintf(latticeFile, "T %u %u %s %s %.3f\n",
                        _arcStarts[i], _arcEnds[i], word, word, _arcWeights[i]);
        }
    }
}

// Update _arcWeights with current LM probabilities.
void
Lattice::UpdateWeights() {
    _arcWeights = _arcBaseWeights;
    for (size_t i = 0; i < _arcProbs.length(); ++i) {
        const ArcNgramIndex &e(_arcProbs[i]);
        _arcWeights[e.arcIndex] -= log(_lm.probs(e.order)[e.ngramIndex]);
    }
    for (size_t i = 0; i < _arcBows.length(); ++i) {
        const ArcNgramIndex &e(_arcBows[i]);
        _arcWeights[e.arcIndex] -= log(_lm.bows(e.order)[e.ngramIndex]);
    }
}

void
Lattice::SetReferenceText(const char *ref) {
    _ref.reset(256);
    size_t      numWords = 0;
    const char *p = ref;
    while (*p != '\0') {
        const char *token = p;
        while (*p != '\0' && !isspace(*p))  ++p;
        if (numWords == _ref.length())
            _ref.resize(_ref.length() * 2);
        if (token != p && !(_skipTags && token[0] == '<'))  // Skip tags
            _ref[numWords++] = _lm.vocab().Find(token, p - token);
        if (*p != '\0') ++p;
    }
    _ref.resize(numWords);

    _FindOraclePath();
}

float
Lattice::ComputeMargin() const {
    // Compute oracle score.
    float oracleScore = 0;
    for (size_t i = 0; i < _oraclePath.length(); ++i)
        oracleScore += _arcWeights[_oraclePath[i]];

    // Compute reverse scores and paths.
    ArcScoreVector bestArcs(_finalNode + 1);
    _ReverseViterbiSearch(bestArcs);

    // Compute margin between oracle and best non-oracle scores.
    if (_IsOracleBestPath(bestArcs)) {
//        vector<float> nbestScores;
//        _FindNBestPaths(bestArcs, 2, nbestScores);
//        return nbestScores[1] - oracleScore;
        return 0;
    } else {
        return bestArcs[0].score - oracleScore;
    }
}

int
Lattice::ComputeWER() const {
    // Compute best hypothesis path.
    ArcScoreVector     bestArcs(_finalNode + 1);
    vector<VocabIndex> hyp;
    _ReverseViterbiSearch(bestArcs);
    _FindBestPath(bestArcs, hyp);

//    for (size_t i = 0; i < _ref.length(); ++i)
//        printf("%s ", _lm.vocab()[_ref[i]]);
//    printf("\n");
//    for (size_t i = 0; i < _oraclePath.length() - 1; ++i)
//        printf("%s ", _lm.vocab()[_arcWords[_oraclePath[i]]]);
//    printf("\n");
//    for (size_t i = 0; i < hyp.size(); ++i)
//        printf("%s ", _lm.vocab()[hyp[i]]);
//    printf("\n");

    if (hyp.size() == 0) return _ref.length();
    if (_ref.length() == 0) return hyp.size();

    // Computes edit distance between hypothesis and reference.
    IntVector nodeScores = Range(_ref.length() + 1);
    for (size_t iHyp = 0; iHyp < hyp.size(); ++iHyp) {
        int prevScore = iHyp;
        nodeScores[0] = iHyp + 1;
        for (size_t iRef = 0; iRef < _ref.length(); ++iRef) {
            int subScore = prevScore + ((hyp[iHyp] == _ref[iRef]) ? 0 : 1);
            prevScore = nodeScores[iRef + 1];
            nodeScores[iRef + 1] = std::min(std::min(
                prevScore + 1,          // Insertion
                nodeScores[iRef] + 1),  // Deletion
                subScore);              // Substitution
        }
    }
//    printf("%s %i %lu\n", _tag.c_str(), nodeScores[_ref.length()], _ref.length());

//    vector<float> nbestScores;
//    _FindNBestPaths(bestArcs, 5, nbestScores);
//    for (size_t i = 0; i < nbestScores.size(); ++i)
//        printf("%f ", nbestScores[i]);
//    printf("\n");

    return nodeScores[_ref.length()];
}

void
Lattice::GetBestPath(vector<VocabIndex> &bestPath) const {
    ArcScoreVector bestArcs(_finalNode + 1);
    _ReverseViterbiSearch(bestArcs);
    _FindBestPath(bestArcs, bestPath);
}


void
Lattice::ComputeForwardScores(FloatVector &nodeScores) const {
    // Compute forward cumulative arc weights.
    nodeScores.reset(_finalNode + 1, -INF);
    nodeScores[0] = 0;
    for (uint i = 0; i < _arcEnds.length(); ++i) {
        assert(_arcStarts[i] < _arcEnds[i]);   // No self-transition.
        nodeScores[_arcEnds[i]] = logAdd(nodeScores[_arcEnds[i]],
            nodeScores[_arcStarts[i]] - _arcWeights[i]);
    }
}

void
Lattice::ComputeBackwardScores(FloatVector &nodeScores) const {
    // Compute backward cumulative arc weights.
    nodeScores.reset(_finalNode + 1);
    NodeIndex currentNode = _finalNode;
    float     totScore    = 0;
    for (uint i = _arcEnds.length() - 1; i != (uint)-1; --i) {
        assert(_arcStarts[i] < _arcEnds[i]);   // No self-transition.
        assert(currentNode >= _arcStarts[i]);  // Sorted arcs.
        if (currentNode != _arcStarts[i]) {
            nodeScores[currentNode] = totScore;
            currentNode = _arcStarts[i];
            totScore    = nodeScores[_arcEnds[i]] - _arcWeights[i];
        } else
            totScore = logAdd(totScore,
                              nodeScores[_arcEnds[i]] - _arcWeights[i]);
    }
    nodeScores[currentNode] = totScore;
}

void
Lattice::ComputePosteriorProbs(const FloatVector &forwardScores,
                               const FloatVector &backwardScores,
                                FloatVector &arcProbs) const {
    float totScore = forwardScores[_finalNode];
    float scoreDiff = fabs(forwardScores[_finalNode] - backwardScores[0]);
    if (scoreDiff > 0.01) {
        printf("ForwardBackwardDiff = %f\n", scoreDiff);
        assert(0);
    }

    arcProbs.reset(_arcEnds.length());
    for (uint i = 0; i < _arcEnds.length(); ++i) {
        arcProbs[i] = exp(forwardScores[_arcStarts[i]] - _arcWeights[i]
                          + backwardScores[_arcEnds[i]] - totScore);
    }
}

void
Lattice::ComputeForwardSteps(const FloatVector &forwardScores,
                             FloatVector &nodeSteps) const {
    // Compute cumulative weighted steps from beginning to each node.
    nodeSteps.reset(_finalNode + 1, -INF);
    for (uint i = 0; i < _arcEnds.length(); ++i) {
        assert(_arcStarts[i] < _arcEnds[i]);   // No self-transition.
        // float pathProb = forwardScores[_arcStarts[i]] * exp(-_arcWeights[i]);
        // float avgSteps = nodeSteps[_arcStarts[i]] / forwardScores[_arcStarts[i]] + 1;
        // nodeSteps[_arcEnds[i]] += pathProb * avgSteps;
        //nodeSteps[_arcEnds[i]] += exp(-_arcWeights[i]) *
        //    (nodeSteps[_arcStarts[i]] + forwardScores[_arcStarts[i]]);
        nodeSteps[_arcEnds[i]] = logAdd(nodeSteps[_arcEnds[i]],
            logAdd(nodeSteps[_arcStarts[i]], forwardScores[_arcStarts[i]])
            - _arcWeights[i]);
    }
}

void
Lattice::ComputeBackwardSteps(const FloatVector &backwardScores,
                              FloatVector &nodeSteps) const {
    // Compute cumulative weighted steps from each node to the end.
    nodeSteps.reset(_finalNode + 1);
    NodeIndex currentNode = _finalNode;
    float     totSteps    = -INF;
    for (uint i = _arcEnds.length() - 1; i != (uint)-1; --i) {
        assert(_arcStarts[i] < _arcEnds[i]);   // No self-transition.
        assert(currentNode >= _arcStarts[i]);  // Sorted arcs.
        if (currentNode != _arcStarts[i]) {
            nodeSteps[currentNode] = totSteps;
            currentNode = _arcStarts[i];
            totSteps    = -INF;
        }
        // float pathProb = backwardScores[_arcEnds[i]] * exp(-_arcWeights[i]);
        // float avgSteps = nodeSteps[_arcEnds[i]] / backwardScores[_arcEnds[i]] + 1;
        // totSteps += pathProb * avgSteps;
        //totSteps += exp(-_arcWeights[i]) *
        //    (nodeSteps[_arcEnds[i]] + backwardScores[_arcEnds[i]]);
        totSteps = logAdd(totSteps,
            logAdd(nodeSteps[_arcEnds[i]], backwardScores[_arcEnds[i]])
            - _arcWeights[i]);
    }
    nodeSteps[currentNode] = totSteps;
}

void
Lattice::EstimateArcPosition(const FloatVector &forwardScores,
                             const FloatVector &backwardScores,
                             FloatVector &nodePositions) const {
    FloatVector forwardSteps, backwardSteps;
    ComputeForwardSteps(forwardScores, forwardSteps);
    ComputeBackwardSteps(backwardScores, backwardSteps);

    nodePositions.reset(_finalNode + 1);
    for (uint i = 0; i < nodePositions.length(); ++i) {
        float avgForwardSteps  = forwardSteps[i] - forwardScores[i];
        float avgBackwardSteps = backwardSteps[i] - backwardScores[i];
        nodePositions[i] = exp(avgForwardSteps -
            logAdd(avgForwardSteps, avgBackwardSteps));
    }
}

float
Lattice::BuildConfusionNetwork() const {
    // Hack: If there are more than a million arcs, we must have low confidence.
    if (_arcStarts.length() > 1000000) {
        printf("#Arcs = %lu\t#RefWords = %lu\n",
               (unsigned long)_arcStarts.length(),
               (unsigned long)_ref.length());
        return 0;
    }

    // Pivot algorithm.
    // Hakkani-Tur, Bechet, Riccardi, and Tur.  Beyond ASR 1-best: Using word
    // confusion networks in spoken language understanding.  Computer Speech
    // and Language 20:4, 2006, 495-514.

    struct Segment {
        Segment(float p, Segment *n=NULL) : endPos(p), baseArc(-1), next(n) { }
        void Insert(float p) { next = new Segment(endPos, next); endPos = p; }

        float            endPos;
        uint             baseArc;
        Segment *        next;
        vector<uint>     arcs;
        vector<WordProb> wordProbs;

    };

    // Compute posterior probabilities and estimate arc positions.
    FloatVector arcProbs, nodePositions;
    {
        FloatVector forwardScores, backwardScores;
        ComputeForwardScores(forwardScores);
        ComputeBackwardScores(backwardScores);
        ComputePosteriorProbs(forwardScores, backwardScores, arcProbs);
        EstimateArcPosition(forwardScores, backwardScores, nodePositions);
    }

    // Build pivot baseline path.
    Segment *pivotPath = new Segment(1);
    {
        Segment *segment = pivotPath;
        ArcScoreVector bestArcs(_finalNode + 1);
        _ReverseViterbiSearch(bestArcs);
        uint bestArc = bestArcs[0].arc;
        while (_arcEnds[bestArc] != _finalNode) {
            segment->baseArc = bestArc;
            segment->Insert(nodePositions[_arcEnds[bestArc]]);
            segment = segment->next;
            bestArc = bestArcs[_arcEnds[bestArc]].arc;
        }
        segment->baseArc = bestArc;
        assert(segment->next == NULL);
    }

    // Add each transition to the pivot structure.
    for (uint i = 0; i < _arcStarts.length(); ++i) {
        // Find pivot segment with the greatest overlap to arc.
        float    arcStartPos = nodePositions[_arcStarts[i]];
        float    arcEndPos   = nodePositions[_arcEnds[i]];
        Segment *segment     = pivotPath;
        while (segment != NULL && segment->endPos < arcStartPos) {
            segment = segment->next;
        }
        if (segment->next != NULL &&
            (segment->endPos - arcStartPos <
            std::min(segment->next->endPos, arcEndPos) - segment->endPos)) {
            segment = segment->next;
        }

        // Does segment contain a transition that precedes the current arc?
        bool splitSegment = false;
        for (uint j = 0; j < segment->arcs.size(); ++j) {
            if (_arcEnds[segment->arcs[j]] == _arcStarts[i]) {
                splitSegment = true;
                break;
            }
        }
        if (splitSegment) {
            // arcStartPos should be within existing segment.
            // Create a new segment with the old endPos set to the arcStartPos.
            segment->Insert(arcStartPos);
            segment = segment->next;
        }
        // Assign arc i to segment.  Will cumulate posterior probs later.
        segment->arcs.push_back(i);
    }

    // Cumulate posterior probs and add epsilon arc across each segment.
    Segment *segment = pivotPath;
    while (segment != NULL) {
        float totProb = 0;
        for (uint i = 0; i < segment->arcs.size(); ++i) {
            float prob = arcProbs[segment->arcs[i]];
            totProb += prob;

            // Check if arc for word already exists.
            VocabIndex word  = _arcWords[segment->arcs[i]];
            bool       found = false;
            for (uint j = 0; j < segment->wordProbs.size(); ++j)
                if (segment->wordProbs[j].word == word) {
                    segment->wordProbs[j].prob += prob;
                    found = true;
                    break;
                }
            if (!found)
                segment->wordProbs.push_back(WordProb(word, prob));
        }
        if (totProb > 1.4) {
            printf("TotalSegmentProb = %f\n", totProb);
            assert(0);
        }
        if (totProb < 1) {
            VocabIndex word = Vocab::EndOfSentence;
            bool       found = false;
            for (uint j = 0; j < segment->wordProbs.size(); ++j)
                if (segment->wordProbs[j].word == word) {
                    segment->wordProbs[j].prob += 1 - totProb;
                    found = true;
                    break;
                }
            if (!found)
                segment->wordProbs.push_back(WordProb(word, 1 - totProb));
        }
        segment = segment->next;
    }

    // Compute average word confidence score.
    float totConfidence = 0;
    int   numWords      = 0;
    segment = pivotPath;
    while (segment != NULL) {
        if (segment->baseArc != (uint)-1) {
            // Only average over words in the best path.
            VocabIndex word = _arcWords[segment->baseArc];
            for (uint j = 0; j < segment->wordProbs.size(); ++j)
                if (segment->wordProbs[j].word == word) {
                    totConfidence += segment->wordProbs[j].prob;
                    numWords++;
                    break;
                }
        }
        segment = segment->next;
    }
    return totConfidence / numWords;
}

void
Lattice::Serialize(FILE *outFile) const {
    WriteHeader(outFile, "Lattice");
    WriteString(outFile, _tag);
    WriteVector(outFile, _arcStarts);
    WriteVector(outFile, _arcEnds);
    WriteVector(outFile, _arcWords);
    WriteVector(outFile, _arcBaseWeights);
    WriteVector(outFile, _ref);
    WriteVector(outFile, _oraclePath);
    WriteVector(outFile, _arcProbs);
    WriteVector(outFile, _arcBows);
    WriteUInt64 (outFile, _oracleWER);
    assert(_arcStarts.length() > 0);
}

void
Lattice::Deserialize(FILE *inFile) {
    VerifyHeader(inFile, "Lattice");
    ReadString(inFile, _tag);
    ReadVector(inFile, _arcStarts);
    ReadVector(inFile, _arcEnds);
    ReadVector(inFile, _arcWords);
    ReadVector(inFile, _arcBaseWeights);
    ReadVector(inFile, _ref);
    ReadVector(inFile, _oraclePath);
    ReadVector(inFile, _arcProbs);
    ReadVector(inFile, _arcBows);
    _oracleWER = ReadUInt64(inFile);

    // Compute _finalNode and _nodeArcs.
    assert(_arcStarts.length() > 0);
    _finalNode = _arcStarts[_arcStarts.length() - 1] + 1;
    _nodeArcs.reset(_finalNode + 2);
    NodeIndex node = (NodeIndex)-1;
    for (uint i = 0; i < _arcStarts.length(); ++i) {
        if (_arcStarts[i] != node) {
            node = _arcStarts[i];
            assert(node < _finalNode);
            _nodeArcs[node] = i;
        }
    }
    _nodeArcs[_finalNode]   = _arcStarts.length();
    _nodeArcs[_finalNode+1] = _arcStarts.length();

    UpdateWeights();
}

template <typename Compare>
void
Lattice::_Sort(size_t numArcs, const Compare &compare) {
    NodeVector sortIndices = Range(numArcs);
    std::sort(sortIndices.begin(), sortIndices.end(), compare);

    // Apply ordered indices to values.
    NodeVector  newArcStarts(numArcs);
    NodeVector  newArcEnds(numArcs);
    VocabVector newArcWords(numArcs);
    FloatVector newArcBaseWeights(numArcs);
    for (size_t i = 0; i < numArcs; ++i) {
        newArcStarts[i]      = _arcStarts[sortIndices[i]];
        newArcEnds[i]        = _arcEnds[sortIndices[i]];
        newArcWords[i]       = _arcWords[sortIndices[i]];
        newArcBaseWeights[i] = _arcBaseWeights[sortIndices[i]];
    }
    _arcStarts.swap(newArcStarts);
    _arcEnds.swap(newArcEnds);
    _arcWords.swap(newArcWords);
    _arcBaseWeights.swap(newArcBaseWeights);
}

void
Lattice::_Reserve(size_t capacity) {
    _arcStarts.resize(capacity);
    _arcEnds.resize(capacity);
    _arcWords.resize(capacity);
    _arcBaseWeights.resize(capacity);
}

void
Lattice::_ComputeArcNgramMapping() {
    // Build node to ngramIndex mapping for lower-order n-grams.
    vector<vector<NgramIndex> > nodeNgramMaps(_lm.order());

    for (size_t o = 1; o < _lm.order(); ++o) {
        const NgramVector  &ngramVector(_lm.model().vectors(o));
        vector<NgramIndex> &map(nodeNgramMaps[o]);
        vector<NgramIndex> &boMap(nodeNgramMaps[o - 1]);
        NodeNgramMap        newNodeMap;

        map.resize(_finalNode, NgramVector::Invalid);
        NgramIndex hist = (o == 1) ? 0 : boMap[0];
        map[0] = ngramVector.Find(hist, Vocab::BeginOfSentence);
        for (size_t i = 0; i < _arcWords.length(); ++i) {
            NgramIndex hist       = (o == 1) ? 0 : boMap[_arcStarts[i]];
            NgramIndex ngramIndex = ngramVector.Find(hist, _arcWords[i]);
            NodeIndex  node       = _arcEnds[i];
            if (node == _finalNode) {
                // Transition to final node.  Do nothing.
            } else if (map[node] == NgramVector::Invalid) {
                // First time visiting this node.  Update map.
                map[node] = ngramIndex;
            } else if (o < _lm.order() && map[node] != ngramIndex) {
                // Previously visited this node with different ngramIndex.
                throw std::runtime_error("FST node n-gram history not unique.");
            }
        }
    }

    // Compute arcIndex to probIndex/bowIndex list.
    vector<ArcNgramIndex> arcProbs;
    vector<ArcNgramIndex> arcBows;
    for (uint i = 0; i < _arcStarts.length(); ++i) {
        for (size_t o = _lm.order(); o > 0; --o) {
            NgramIndex hist = (o == 1) ? 0 : nodeNgramMaps[o-1][_arcStarts[i]];
            if (hist != NgramVector::Invalid) {
                NgramIndex indx = (o < _lm.order() && _arcEnds[i] != _finalNode)
                    ? nodeNgramMaps[o][_arcEnds[i]]
                    : _lm.model().vectors(o).Find(hist, _arcWords[i]);
                if (indx == NgramVector::Invalid) {
                    arcBows.push_back(ArcNgramIndex(i, o - 1, hist));
                    assert(o != 1);  // Out of vocabulary word found in lattice.
                } else {
                    arcProbs.push_back(ArcNgramIndex(i, o, indx));
                    break;
                }
            }
        }
    }
    _arcProbs = arcProbs;
    _arcBows  = arcBows;
}

// Return the lattice path that yields the lowest WER and score.
float
Lattice::_FindOraclePath() {
    // Best first search.
    unordered_map<Backtrace, Backtrace, BacktraceHash> bestParent;
    std::priority_queue<OraclePath>                    paths;
    paths.push(OraclePath(0, 0, -1, 0, 0));
    while (!paths.empty()) {
        OraclePath path     = paths.top();
        int        wer      = path.wer;
        float      score    = path.score;
        uint       arcIndex = path.arcIndex;
        uint       refIndex = path.refIndex;
        NodeIndex  node     = (arcIndex == (uint)-1) ? 0 : _arcEnds[arcIndex];
        paths.pop();

        // Skip previously visited nodes.
        Backtrace backtrace(node, refIndex);
        if (bestParent.find(backtrace) == bestParent.end()) {
            bestParent[backtrace] = Backtrace(
                (arcIndex == (uint)-1) ? 0 : _arcStarts[arcIndex],
                path.prevRefIndex);

            // Check if complete path.
            if (node == _finalNode && path.refIndex == _ref.length()) {
                vector<uint> path;
                Backtrace startTrace(0, 0);
                while (backtrace != startTrace) {
                    backtrace = bestParent[backtrace];
                    if (backtrace.first != node) {
                        // Find arc from backtrace.first -> node.
                        arcIndex = _nodeArcs[backtrace.first];
                        while (_arcEnds[arcIndex] != node) ++arcIndex;
                        path.push_back(arcIndex);
                        node = backtrace.first;
                    }
                }
                _oraclePath.reset(path.size());
                std::copy(path.rbegin(), path.rend(), _oraclePath.begin());
                _oracleWER = wer;
                return score;
            }

            // Expand path.
            if (refIndex < _ref.length()) {
                // Match / substitution.
                for (uint i = _nodeArcs[node]; i < _nodeArcs[node + 1]; ++i)
                    paths.push(OraclePath(
                        wer + ((_arcWords[i] == _ref[refIndex]) ? 0 : 1),
                        score + _arcWeights[i], i, refIndex, refIndex+1));

                // Deletion.
                paths.push(OraclePath(wer + 1, score, arcIndex,
                                      refIndex, refIndex + 1));
            }

            // Insertions.
            for (uint i = _nodeArcs[node]; i < _nodeArcs[node + 1]; ++i)
                paths.push(OraclePath(wer + 1, score + _arcWeights[i],
                                      i, refIndex, refIndex));

            // Epsilon traversal.
            for (uint i = _nodeArcs[node]; i < _nodeArcs[node + 1]; ++i)
                if (_arcWords[i] == 0)
                    paths.push(OraclePath(wer, score + _arcWeights[i],
                                          i, refIndex, refIndex));
        }
    }
    throw std::runtime_error("Unexpected error.");
}

void
Lattice::_ReverseViterbiSearch(ArcScoreVector &bestArcs) const {
    // Perform reverse Viterbi search to find best path.
    NodeIndex currentNode = _finalNode;
    uint      bestArc     = (uint)-1;
    float     bestScore   = 0;
    for (uint i = _arcEnds.length() - 1; i != (uint)-1; --i) {
        assert(_arcStarts[i] < _arcEnds[i]);   // No self-transition.
        assert(currentNode >= _arcStarts[i]);  // Sorted arcs.
        if (currentNode != _arcStarts[i]) {
            bestArcs[currentNode] = ArcScore(bestArc, bestScore);
            currentNode = _arcStarts[i];
            bestArc     = i;
            bestScore   = bestArcs[_arcEnds[i]].score + _arcWeights[i];
        } else {
            float pathScore = bestArcs[_arcEnds[i]].score + _arcWeights[i];
            if (pathScore < bestScore) {
                bestArc   = i;
                bestScore = pathScore;
            }
        }
    }
    bestArcs[currentNode] = ArcScore(bestArc, bestScore);
}

float
Lattice::_FindBestPath(const ArcScoreVector &bestArcs,
                       vector<VocabIndex> &bestPath) const {
    // Follow _bestNodeArcs to find best path.
    uint bestArc = bestArcs[0].arc;
    bestPath.clear();
    while (_arcEnds[bestArc] != _finalNode) {
        VocabIndex word = _arcWords[bestArc];
//        printf("%f ", _arcWeights[bestArc]);
        if (!_skipTags || _lm.vocab()[word][0] != '<')  // Skip tag words.
            bestPath.push_back(_arcWords[bestArc]);
        bestArc = bestArcs[_arcEnds[bestArc]].arc;
    }
//    printf("\n");
    return bestArcs[0].score;
}

void
Lattice::_FindNBestPaths(const ArcScoreVector &bestArcs,
                         size_t n,
                         vector<float> &nbestScores) const {
    // Perform forward A* search using bestNodeScore as future scores.
    // Priority queue stores the transition index and forward score up to end.
    // To find n-best, do not prune paths.
    nbestScores.clear();
    std::priority_queue<Path> paths;
    for (uint i = _nodeArcs[0]; i < _nodeArcs[1]; ++i) {
        NodeIndex node = _arcEnds[i];
        paths.push(Path(_arcWeights[i] + bestArcs[node].score, node));
    }
    while (!paths.empty()) {
        Path      path = paths.top();
        NodeIndex node = path.node;
        paths.pop();
        if (node == _finalNode) {
            nbestScores.push_back(path.score);
            if (--n == 0) return;
        } else {
            float baseScore = path.score - bestArcs[node].score;
            for (uint i = _nodeArcs[node]; i < _nodeArcs[node + 1]; ++i) {
                float score = baseScore + _arcWeights[i] +
                              bestArcs[_arcEnds[i]].score;
                paths.push(Path(score, _arcEnds[i]));
            }
        }
    }
}

bool
Lattice::_IsOracleBestPath(const ArcScoreVector &bestArcs) const {
    uint bestArc = bestArcs[0].arc;
    for (size_t i = 0; i < _oraclePath.length(); ++i) {
        if (_oraclePath[i] != bestArc)
            return false;
        bestArc = bestArcs[_arcEnds[bestArc]].arc;
    }
    return true;
}
