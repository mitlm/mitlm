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

#include <vector>
#include "util/CommandOptions.h"
#include "util/ZFile.h"
#include "util/Logger.h"
#include "Types.h"
#include "NgramLM.h"
#include "Smoothing.h"
#include "PerplexityOptimizer.h"
#include "WordErrorRateOptimizer.h"

#ifdef F77_DUMMY_MAIN
#  ifdef __cplusplus
extern "C"
#  endif
int F77_DUMMY_MAIN () { return 1; }
#endif

using std::vector;
using std::string;

////////////////////////////////////////////////////////////////////////////////

const char *headerDesc =
"Usage: estimate-ngram [Options]\n"
"\n"
"Estimates an n-gram language model by cumulating n-gram count statistics,\n"
"smoothing observed counts, and building a backoff n-gram model.  Parameters\n"
"can be optionally tuned to optimize development set performance.\n"
"\n"
"Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),\n"
"or '-' to indicate stdin/stdout.\n";

const char *footerDesc =
"---------------------------------------------------------------\n"
"| MIT Language Modeling Toolkit (v0.4)                        |\n"
"| Copyright (C) 2009 Bo-June (Paul) Hsu                       |\n"
"| MIT Computer Science and Artificial Intelligence Laboratory |\n"
"---------------------------------------------------------------\n";

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    // Parse command line options.
    mitlm::CommandOptions opts(headerDesc, footerDesc);
    opts.AddOption("h,help", "Print this message.");
    opts.AddOption("verbose", "Set verbosity level.", "1", "int");
    opts.AddOption("o,order", "Set the n-gram order of the estimated LM.", "3", "int");
    opts.AddOption("v,vocab", "Fix the vocab to only words from the specified file.", NULL, "file");
    opts.AddOption("u,unk", "Replace all out of vocab words with <unk>.", "false", "boolean");
    opts.AddOption("t,text", "Add counts from text files.", NULL, "files");
    opts.AddOption("c,counts", "Add counts from counts files.", NULL, "files");
    opts.AddOption("s,smoothing", "Specify smoothing algorithms.", "ModKN", "ML, FixKN, FixModKN, FixKN#, KN, ModKN, KN#");
    opts.AddOption("wf,weight-features", "Specify n-gram weighting features.", NULL, "features-template");
    opts.AddOption("p,params", "Set initial model params.", NULL, "file");
    opts.AddOption("oa,opt-alg", "Specify optimization algorithm.", "Powell", "Powell, LBFGS, LBFGSB");
    opts.AddOption("op,opt-perp", "Tune params to minimize dev set perplexity.", NULL, "file");
    opts.AddOption("ow,opt-wer", "Tune params to minimize lattice word error rate.", NULL, "file");
    opts.AddOption("om,opt-margin", "Tune params to minimize lattice margin.", NULL, "file");
    opts.AddOption("wb,write-binary", "Write LM/counts files in binary format.", "false", "boolean");
    opts.AddOption("wp,write-params", "Write tuned model params to file.", NULL, "file");
    opts.AddOption("wv,write-vocab", "Write LM vocab to file.", NULL, "file");
    opts.AddOption("wc,write-counts", "Write n-gram counts to file.", NULL, "file");
    opts.AddOption("wec,write-eff-counts", "Write effective n-gram counts to file.", NULL, "file");
    opts.AddOption("wlc,write-left-counts", "Write left-branching n-gram counts to file.", NULL, "file");
    opts.AddOption("wrc,write-right-counts", "Write right-branching n-gram counts to file.", NULL, "file");
    opts.AddOption("wl,write-lm", "Write ARPA backoff LM to file.", NULL, "file");
    opts.AddOption("ep,eval-perp", "Compute test set perplexity.", NULL, "files");
    opts.AddOption("ew,eval-wer", "Compute test set lattice word error rate.", NULL, "files");
    opts.AddOption("em,eval-margin", "Compute test set lattice margin.", NULL, "files");
    if (!opts.ParseArguments(argc, (const char **)argv) ||
        opts["help"] != NULL) {
        std::cout << std::endl;
        opts.PrintHelp();
        return 1;
    }

    // Process basic command line arguments.
    size_t order = atoi(opts["order"]);
    bool writeBinary = mitlm::AsBoolean(opts["write-binary"]);
    mitlm::Logger::SetVerbosity(atoi(opts["verbose"]));

    if (!opts["text"] && !opts["counts"]) {
        mitlm::Logger::Error(1, "Specify training data using -text or -counts.\n");
        exit(1);
    }

    // Build language model.
    mitlm::NgramLM lm(order);
    lm.Initialize(opts["vocab"], mitlm::AsBoolean(opts["unk"]),
                  opts["text"], opts["counts"], 
                  opts["smoothing"], opts["weight-features"]);

    // Estimate LM.
    mitlm::ParamVector params(lm.defParams());
    if (opts["params"]) {
        mitlm::Logger::Log(1, "Loading parameters from %s...\n", opts["params"]);
        mitlm::ZFile f(opts["params"], "r");
        VerifyHeader(f, "Param");
        ReadVector(f, params);
        if (params.length() != lm.defParams().length()) {
            mitlm::Logger::Error(1, "Number of parameters mismatched.\n");
            exit(1);
        }
    }

    mitlm::Optimization optAlg = mitlm::ToOptimization(opts["opt-alg"]);
    if (optAlg == mitlm::UnknownOptimization) {
        mitlm::Logger::Error(1, "Unknown optimization algorithms '%s'.\n",
                      opts["opt-alg"]);
        exit(1);
    }
        
    if (opts["opt-perp"]) {
        if (params.length() == 0) {
            mitlm::Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            mitlm::Logger::Log(1, "Loading development set %s...\n", opts["opt-perp"]);
            mitlm::ZFile devZFile(opts["opt-perp"]);
            mitlm::PerplexityOptimizer dev(lm, order);
            dev.LoadCorpus(devZFile);

            mitlm::Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optEntropy = dev.Optimize(params, optAlg);
            mitlm::Logger::Log(2, " Best perplexity = %f\n", exp(optEntropy));
        }
    }
    if (opts["opt-margin"]) {
        if (params.length() == 0) {
            mitlm::Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            mitlm::Logger::Log(1, "Loading development lattices %s...\n",
                        opts["opt-margin"]);
            mitlm::ZFile devZFile(opts["opt-margin"]);
            mitlm::WordErrorRateOptimizer dev(lm, order);
            dev.LoadLattices(devZFile);

            mitlm::Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optMargin = dev.OptimizeMargin(params, optAlg);
            mitlm::Logger::Log(2, " Best margin = %f\n", optMargin);
        }
    }
    if (opts["opt-wer"]) {
        if (params.length() == 0) {
            mitlm::Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            mitlm::Logger::Log(1, "Loading development lattices %s...\n",
                        opts["opt-wer"]);
            mitlm::ZFile devZFile(opts["opt-wer"]);
            mitlm::WordErrorRateOptimizer dev(lm, order);
            dev.LoadLattices(devZFile);

            mitlm::Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optWER = dev.OptimizeWER(params, optAlg);
            mitlm::Logger::Log(2, " Best WER = %f%%\n", optWER);
        }
    }

    // Estimate full model.
    if (opts["write-lm"] || opts["eval-perp"] || 
        opts["eval-margin"] || opts["eval-wer"]) {
        mitlm::Logger::Log(1, "Estimating full n-gram model...\n");
        lm.Estimate(params);
    }

    // Save results.
    if (opts["write-params"]) {
        mitlm::Logger::Log(1, "Saving parameters to %s...\n", opts["write-params"]);
        mitlm::ZFile f(opts["write-params"], "w");
        WriteHeader(f, "Param");
        WriteVector(f, params);
    }
    if (opts["write-vocab"]) {
        mitlm::Logger::Log(1, "Saving vocabulary to %s...\n", opts["write-vocab"]);
        mitlm::ZFile vocabZFile(opts["write-vocab"], "w");
        lm.SaveVocab(vocabZFile);
    }
    if (opts["write-counts"]) {
        mitlm::Logger::Log(1, "Saving counts to %s...\n", opts["write-counts"]);
        mitlm::ZFile countZFile(opts["write-counts"], "w");
        lm.SaveCounts(countZFile, writeBinary);
    }
    if (opts["write-eff-counts"]) {
        mitlm::Logger::Log(1, "Saving effective counts to %s...\n",
                    opts["write-eff-counts"]);
        mitlm::ZFile countZFile(opts["write-eff-counts"], "w");
        lm.SaveEffCounts(countZFile, writeBinary);
    }
    if (opts["write-left-counts"]) {
        mitlm::Logger::Log(1, "Saving left-branching counts to %s...\n",
                    opts["write-left-counts"]);
        mitlm::ZFile countZFile(opts["write-left-counts"], "w");

        vector<mitlm::CountVector> countVectors(order + 1);
        for (size_t o = 0; o < order; ++o) {
            countVectors[o].reset(lm.sizes(o), 0);
            BinCount(lm.backoffs(o+1), countVectors[o]);
        }
        lm.model().SaveCounts(countVectors, countZFile, true);
    }
    if (opts["write-right-counts"]) {
        mitlm::Logger::Log(1, "Saving right-branching counts to %s...\n",
                    opts["write-right-counts"]);
        mitlm::ZFile countZFile(opts["write-right-counts"], "w");

        vector<mitlm::CountVector> countVectors(order + 1);
        for (size_t o = 0; o < order; ++o) {
            countVectors[o].reset(lm.sizes(o), 0);
            BinCount(lm.hists(o+1), countVectors[o]);
        }
        lm.model().SaveCounts(countVectors, countZFile, true);
    }
    if (opts["write-lm"]) {
        mitlm::Logger::Log(1, "Saving LM to %s...\n", opts["write-lm"]);
        mitlm::ZFile lmZFile(opts["write-lm"], "w");
        lm.SaveLM(lmZFile, writeBinary);
    }

    // Evaluate LM.
    if (opts["eval-perp"]) {
        mitlm::Logger::Log(0, "Perplexity Evaluations:\n");
        vector<string> evalFiles;
        mitlm::trim_split(evalFiles, opts["eval-perp"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            mitlm::Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            mitlm::ZFile evalZFile(evalFiles[i].c_str());
            mitlm::PerplexityOptimizer eval(lm, order);
            eval.LoadCorpus(evalZFile);

            mitlm::Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputePerplexity(params));
        }
    }
    if (opts["eval-margin"]) {
        mitlm::Logger::Log(0, "Margin Evaluations:\n");
        vector<string> evalFiles;
        mitlm::trim_split(evalFiles, opts["eval-margin"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            mitlm::Logger::Log(1, "Loading eval lattices %s...\n",
                        evalFiles[i].c_str());
            mitlm::ZFile evalZFile(evalFiles[i].c_str());
            mitlm::WordErrorRateOptimizer eval(lm, order);
            eval.LoadLattices(evalZFile);

            mitlm::Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputeMargin(params));
        }
    }
    if (opts["eval-wer"]) {
        mitlm::Logger::Log(0, "WER Evaluations:\n");
        vector<string> evalFiles;
        mitlm::trim_split(evalFiles, opts["eval-wer"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            mitlm::Logger::Log(1, "Loading eval lattices %s...\n",
                        evalFiles[i].c_str());
            mitlm::ZFile evalZFile(evalFiles[i].c_str());
            mitlm::WordErrorRateOptimizer eval(lm, order);
            eval.LoadLattices(evalZFile);

            mitlm::Logger::Log(0, "\t%s\t%.2f%%\n", evalFiles[i].c_str(),
                        eval.ComputeWER(params));
        }
    }

    return 0;
}

