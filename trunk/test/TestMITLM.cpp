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

#include <gtest/gtest.h>
#include "optimize/Optimization.h"
#include "util/BitOps.h"
#include "util/FastHash.h"
#include "util/ZFile.h"
#include "Vocab.h"
#include "NgramVector.h"
#include "NgramModel.h"
#include "NgramLM.h"
#include "Lattice.h"
#include "Timer.h"

////////////////////////////////////////////////////////////////////////////////

TEST(Utilities, BitOps) {
    EXPECT_EQ(0u, fls(0));
    EXPECT_EQ(1u, fls(1));
    EXPECT_EQ(2u, fls(2));
    EXPECT_EQ(2u, fls(3));
    EXPECT_EQ(3u, fls(4));
    EXPECT_EQ(3u, fls(5));
    EXPECT_EQ(3u, fls(6));
    EXPECT_EQ(3u, fls(7));
    EXPECT_EQ(4u, fls(8));
    EXPECT_EQ(4u, fls(9));
    EXPECT_EQ(31u, fls(0x7FFFFFFF));
    EXPECT_EQ(32u, fls(0xF0000000));
    EXPECT_EQ(32u, fls(0xFFFFFFFF));
}

TEST(Utilities, HashFunc) {
    Vocab vocab;
    vocab.LoadVocab(ZFile("data/switchboard.vocab"));

    int superFastTotal = 0;
    START_TIME_N(vocab.size()) {
        for (VocabIndex i = 0; i < (VocabIndex)vocab.size(); i++) {
            superFastTotal += SuperFastHash(vocab[i], vocab.wordlen(i));
        }
    } END_TIME(timeSuperFastHash)

    int srilmTotal = 0;
    START_TIME_N(vocab.size()) {
        for (VocabIndex i = 0; i < (VocabIndex)vocab.size(); i++)
            srilmTotal += StringHash(vocab[i], vocab.wordlen(i));
    } END_TIME(timeSRILM)

    printf("SuperFastHash = %.2f,\tSRILMHash = %.2f\n",
           timeSuperFastHash, timeSRILM);
    if (superFastTotal + srilmTotal == 0) printf("\n");
}

TEST(Vocab, Basics) {
    Vocab vocab(8);
    EXPECT_EQ(2, vocab.Add("the"));
    EXPECT_EQ(3, vocab.Add("a"));
    EXPECT_EQ(4, vocab.Add("<background>"));
    EXPECT_EQ(0, vocab.Find("</s>"));
    EXPECT_EQ(1, vocab.Find("<s>"));
    EXPECT_EQ(2, vocab.Find("the"));
    EXPECT_EQ(3, vocab.Find("a"));
    EXPECT_EQ(4, vocab.Find("<background>"));
    EXPECT_STREQ("</s>", vocab[0]);
    EXPECT_STREQ("<s>", vocab[1]);
    EXPECT_STREQ("the", vocab[2]);
    EXPECT_STREQ("a", vocab[3]);
    EXPECT_STREQ("<background>", vocab[4]);
    EXPECT_EQ(4u, vocab.wordlen(0));
    EXPECT_EQ(3u, vocab.wordlen(1));
    EXPECT_EQ(3u, vocab.wordlen(2));
    EXPECT_EQ(1u, vocab.wordlen(3));
    EXPECT_EQ(12u, vocab.wordlen(4));
    EXPECT_EQ(5u, vocab.size());

    VocabVector vocabMapping;
    vocab.Sort(vocabMapping);
    EXPECT_STREQ("</s>", vocab[0]);
    EXPECT_STREQ("<s>", vocab[1]);
    EXPECT_STREQ("<background>", vocab[2]);
    EXPECT_STREQ("a", vocab[3]);
    EXPECT_STREQ("the", vocab[4]);
}

TEST(Vocab, InputOutput) {
    Vocab vocab;

    START_TIME {
        vocab.LoadVocab(ZFile("data/switchboard.vocab"));
    } END_TIME(timeLoad)

    START_TIME {
      vocab.SaveVocab(ZFile("data/switchboard.vocab.out", "w"));
    } END_TIME(timeSave)

    printf("Load = %.2f,\tSave = %.2f\n",
           timeLoad / vocab.size(), timeSave / vocab.size());
}

TEST(Vocab, BinaryInputOutput) {
    Vocab vocab;

    START_TIME {
        vocab.LoadVocab(ZFile("data/switchboard.vocab"));
    } END_TIME(timeLoad)

    START_TIME {
      vocab.SaveVocab(ZFile("data/switchboard.vocab.bin", "wb"), true);
    } END_TIME(timeSave)

    START_TIME {
        vocab.LoadVocab(ZFile("data/switchboard.vocab.bin", "rb"));
    } END_TIME(timeLoad2)

    START_TIME {
      vocab.SaveVocab(ZFile("data/switchboard.vocab.out", "w"));
    } END_TIME(timeSave2)

    printf("Load = %.2f, %.2f,\tSave = %.2f, %.2f\n",
           timeLoad / vocab.size(), timeLoad2 / vocab.size(),
           timeSave / vocab.size(), timeSave2 / vocab.size());
}

TEST(NgramVector, Basics) {
    NgramVector v;
    EXPECT_EQ(0, v.Add(0, 0));
    EXPECT_EQ(0, v.Add(0, 0));
    EXPECT_EQ(1, v.Add(0, 1));
    EXPECT_EQ(0, v.Add(0, 0));
    EXPECT_EQ(1, v.Add(0, 1));
    EXPECT_EQ(2, v.Add(1, 0));
}

TEST(NgramModel, LoadCorpus) {
    NgramModel model;
    vector<CountVector> countVectors;

    START_TIME {
        countVectors.resize(0);
        ZFile corpusFile("data/switchboard.txt", "r");
        model.LoadCorpus(countVectors, corpusFile);
    } END_TIME(timeLoad)

    START_TIME {
        ZFile countsFile("data/switchboard.counts.out", "w");
        model.SaveCounts(countVectors, countsFile);
    } END_TIME(timeSave)

    printf("Load = %.2fs,\tSave = %.2fs\n",
           timeLoad / 2660000000.0, timeSave / 2660000000.0);
}

TEST(NgramModel, LoadCounts) {
    NgramModel model;
    vector<CountVector> countVectors;

    START_TIME {
        countVectors.resize(0);
        ZFile countsFile("data/switchboard.counts", "r");
        model.LoadCounts(countVectors, countsFile);
    } END_TIME(timeLoad)

    START_TIME {
        ZFile countsFile("data/switchboard.counts.out", "w");
        model.SaveCounts(countVectors, countsFile);
    } END_TIME(timeSave)

    printf("Load = %.2fs,\tSave = %.2fs\n",
           timeLoad / 2660000000.0, timeSave / 2660000000.0);
}

TEST(NgramModel, LoadLM) {
    NgramModel model;
    vector<ProbVector> probVectors;
    vector<ProbVector> bowVectors;

    START_TIME {
        ZFile lmFile("data/switchboard.lm", "r");
        model.LoadLM(probVectors, bowVectors, lmFile);
    } END_TIME(timeLoad)

    START_TIME {
        ZFile lmFile("data/switchboard.lm.out", "w");
        model.SaveLM(probVectors, bowVectors, lmFile);
    } END_TIME(timeSave)

    printf("Load = %.2fs,\tSave = %.2fs\n",
           timeLoad / 2660000000.0, timeSave / 2660000000.0);
}

struct Rosenbrock {
    int numCalls;
    Rosenbrock() : numCalls(0) { }
    Param operator()(const ParamVector &v) {
        numCalls++;
        Param f = 0;
        for (size_t i = 0; i < v.length() - 1; i++) {
            Param t1 = 1 - v[i];
            Param t2 = v[i+1] - v[i]*v[i];
            f += t1*t1 + 100*t2*t2;
        }
        return f;
    }
};

TEST(Optimization, Rosenbrock) {
    Rosenbrock  ros;
    ParamVector x, xOrig(2, 0);
    Param       fMin;
    int         numIter;
    START_TIME {
        x = xOrig;
        ros.numCalls = 0;
        fMin = MinimizePowell(ros, x, numIter);
    } END_TIME(timeOptimize)
    printf("Optimize = %.2f\n", timeOptimize);
    EXPECT_NEAR(0.0, fMin, 1e-20);
    for (size_t i = 0; i < x.length(); ++i)
        EXPECT_NEAR(1.0, x[i], 1e-10);
    EXPECT_EQ(9, numIter);
    EXPECT_EQ(292, ros.numCalls);
}

TEST(Optimization, RosenbrockLBFGS) {
    Rosenbrock  ros;
    ParamVector x, xOrig(2, 0);
    Param       fMin;
    int         numIter;
    START_TIME {
        x = xOrig;
        ros.numCalls = 0;
        fMin = MinimizeLBFGS(ros, x, numIter, 1e-14, 1e-10);
    } END_TIME(timeOptimize)
    printf("Optimize = %.2f\n", timeOptimize);
    EXPECT_NEAR(0.0, fMin, 1e-20);
    for (size_t i = 0; i < x.length(); ++i)
        EXPECT_NEAR(1.0, x[i], 1e-10);
    EXPECT_EQ(30, numIter);
    EXPECT_EQ(93, ros.numCalls);
}

TEST(Optimization, RosenbrockLBFGSB) {
    Rosenbrock  ros;
    ParamVector x, xOrig(2, 0);
    Param       fMin;
    int         numIter;
    START_TIME {
        x = xOrig;
        ros.numCalls = 0;
        fMin = MinimizeLBFGSB(ros, x, numIter, 1e-14, 1e4, 1e-7);
    } END_TIME(timeOptimize)
    printf("Optimize = %.2f\n", timeOptimize);
    EXPECT_NEAR(0.0, fMin, 1e-20);
    for (size_t i = 0; i < x.length(); ++i)
        EXPECT_NEAR(1.0, x[i], 1e-10);
    EXPECT_EQ(20, numIter);
    EXPECT_EQ(75, ros.numCalls);
}

TEST(Lattice, Basic) {
    ArpaNgramLM lm(1);
    lm.LoadLM(ZFile("data/lattice.lm", "r"));

    Lattice l(lm);
    l.LoadLattice(ZFile("data/lattice.fst", "r"));
    l.SaveLattice(ZFile("data/lattice.fst.out", "w"));

//    VocabVector bestPath;
//    float bestScore = l.FindBestPath(bestPath);
//    EXPECT_FLOAT_EQ(5.0f, bestScore);
//    ASSERT_EQ(3ul, bestPath.length());
//    EXPECT_STREQ("b", lm.vocab()[bestPath[0]]);
//    EXPECT_STREQ("h", lm.vocab()[bestPath[1]]);
//    EXPECT_STREQ("n", lm.vocab()[bestPath[2]]);
//
//    vector<float> nbestScores;
//    l.FindNBestPaths(10, nbestScores);
//    ASSERT_EQ(7ul, nbestScores.size());
//    EXPECT_FLOAT_EQ(5.0f, nbestScores[0]);
//    EXPECT_FLOAT_EQ(6.0f, nbestScores[1]);
//    EXPECT_FLOAT_EQ(7.0f, nbestScores[2]);
//    EXPECT_FLOAT_EQ(7.0f, nbestScores[3]);
//    EXPECT_FLOAT_EQ(8.0f, nbestScores[4]);
//    EXPECT_FLOAT_EQ(8.0f, nbestScores[5]);
//    EXPECT_FLOAT_EQ(10.0f, nbestScores[6]);
//
//    l.SetReferenceText("b h n");
//    EXPECT_EQ(0, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("");
//    EXPECT_EQ(3, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("b h a");
//    EXPECT_EQ(1, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("b h");
//    EXPECT_EQ(1, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("a b h");
//    EXPECT_EQ(2, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("a b h n");
//    EXPECT_EQ(1, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("b f l");
//    EXPECT_EQ(2, l.ComputeWER());
//    EXPECT_FLOAT_EQ(7.0f, l.FindOraclePath());
//    EXPECT_FALSE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(-2.0f, l.ComputeMargin());
//
//    l.SetReferenceText("b m");
//    EXPECT_EQ(2, l.ComputeWER());
//    EXPECT_FLOAT_EQ(5.0f, l.FindOraclePath());
//    EXPECT_TRUE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(1.0f, l.ComputeMargin());
//
//    l.SetReferenceText("b k m");
//    EXPECT_EQ(2, l.ComputeWER());
//    EXPECT_FLOAT_EQ(7.0f, l.FindOraclePath());
//    EXPECT_FALSE(l.IsOracleBestPath());
//    EXPECT_FLOAT_EQ(-2.0f, l.ComputeMargin());

}

TEST(Lattice, UpdateWeights) {
    ArpaNgramLM lm;
    lm.LoadLM(ZFile("data/lattice.lm", "r"));

    Lattice l(lm);
    l.LoadLattice(ZFile("data/lattice.wrd.fst", "r"));
    l.UpdateWeights();
    l.SaveLattice(ZFile("data/lattice.wrd.fst.out", "w"));
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
