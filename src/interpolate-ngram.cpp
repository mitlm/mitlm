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
#include "Smoothing.h"
#include "NgramLM.h"
#include "InterpolatedNgramLM.h"
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
"Usage: interpolate-ngram [Options]\n"
"\n"
"Interpolates multiple n-gram models by computing appropriate interpolation\n"
"weights from optional features and constructing a statically interpolated\n"
"n-gram model.  Parameters can be optionally tuned to optimize development set\n"
"performance.\n"
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
    CommandOptions opts(headerDesc, footerDesc);
    opts.AddOption("h,help", "Print this message.");
    opts.AddOption("verbose", "Set verbosity level.", "1", "int");
    opts.AddOption("o,order", "Set the n-gram order of the estimated LM.", "3", "int");
    opts.AddOption("v,vocab", "Fix the vocab to only words from the specified file.", NULL, "file");
    opts.AddOption("u,unk", "Replace all out of vocab words with <unk>.", "false", "boolean");
    opts.AddOption("l,lm", "Interpolate specified LM files.", NULL, "file");
    opts.AddOption("t,text", "Interpolate models trained from text files.", NULL, "files");
    opts.AddOption("c,counts", "Interpolate models trained from counts files.", NULL, "files");
    opts.AddOption("s,smoothing", "Specify smoothing algorithms.", "ModKN", "ML, FixKN, FixModKN, FixKN#, KN, ModKN, KN#");
    opts.AddOption("wf,weight-features", "Specify n-gram weighting features.", NULL, "features-template");
    opts.AddOption("i,interpolation", "Specify interpolation mode.", "LI", "LI, CM, GLI");
    opts.AddOption("if,interpolation-features", "Specify interpolation features.", NULL, "features-template");
    opts.AddOption("tpo,tie-param-order", "Tie parameters across n-gram order.", "true", "boolean");
    opts.AddOption("tpl,tie-param-lm", "Tie parameters across LM components.", "false", "boolean");
    opts.AddOption("p,params", "Set initial model params.", NULL, "file");
    opts.AddOption("oa,opt-alg", "Specify optimization algorithm.", "LBFGS", "Powell, LBFGS, LBFGSB");
    opts.AddOption("op,opt-perp", "Tune params to minimize dev set perplexity.", NULL, "file");
    opts.AddOption("ow,opt-wer", "Tune params to minimize lattice word error rate.", NULL, "file");
    opts.AddOption("om,opt-margin", "Tune params to minimize lattice margin.", NULL, "file");
    opts.AddOption("wb,write-binary", "Write LM/counts files in binary format.", "false", "boolean");
    opts.AddOption("wp,write-params", "Write tuned model params to file.", NULL, "file");
    opts.AddOption("wv,write-vocab", "Write LM vocab to file.", NULL, "file");
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
    bool writeBinary = AsBoolean(opts["write-binary"]);
    Logger::SetVerbosity(atoi(opts["verbose"]));

    // Read language models.
    vector<SharedPtr<NgramLMBase> > lms;
    vector<string> corpusFiles;
    if (opts["text"] || opts["counts"]) {
        if (opts["text"] && opts["counts"]) {
            Logger::Error(1, "Cannot specify both -text and -counts.\n");
            exit(1);
        }

        vector<string> smoothings;
        vector<string> features;
        bool           fromText = opts["text"];
        if (fromText)
            trim_split(corpusFiles, opts["text"], ',');
        else
            trim_split(corpusFiles, opts["counts"], ',');
        trim_split(smoothings, opts["smoothing"], ';');
        trim_split(features, opts["weight-features"], ';');

        if ((smoothings.size() != 1 && smoothings.size() != corpusFiles.size()) ||
            (features.size() > 1 && features.size() != corpusFiles.size())) {
            Logger::Error(1, "Inconsistent number of LM components.\n");
            exit(1);
        }

        for (size_t i = 0; i < corpusFiles.size(); i++) {
            NgramLM *pLM = new NgramLM(order);
            pLM->Initialize(opts["vocab"], AsBoolean(opts["unk"]), 
                            fromText ? corpusFiles[i].c_str() : NULL, 
                            fromText ? NULL : corpusFiles[i].c_str(), 
                            GetItem(smoothings, i), GetItem(features, i));
            lms.push_back((SharedPtr<NgramLMBase>)pLM);
        }
    }

    // Read component language model input files.
    if (opts["lm"]) {
        vector<string> lmFiles;
        trim_split(lmFiles, opts["lm"], ',');
        for (size_t l = 0; l < lmFiles.size(); l++) {
            Logger::Log(1, "Loading component LM %s...\n", lmFiles[l].c_str());
            ArpaNgramLM *pLM = new ArpaNgramLM(order);
            if (opts["vocab"]) {
                ZFile vocabZFile(opts["vocab"]);
                pLM->LoadVocab(vocabZFile);
                if (AsBoolean(opts["unk"])) {
                    Logger::Error(1, "-unk with -lm is not implemented yet.\n");
                    exit(1);
                }
            }
            ZFile lmZFile(lmFiles[l].c_str(), "r");
            pLM->LoadLM(lmZFile);
            lms.push_back((SharedPtr<NgramLMBase>)pLM);
            corpusFiles.push_back(lmFiles[l]);
        }
    }

    // Interpolate language models.
    Logger::Log(1, "Interpolating component LMs...\n");
    if (AsBoolean(opts["tie-param-order"]))
        Logger::Log(1, "Tying parameters across n-gram order...\n");
    if (AsBoolean(opts["tie-param-lm"]))
        Logger::Log(1, "Tying parameters across LM components...\n");
    InterpolatedNgramLM ilm(order, AsBoolean(opts["tie-param-order"]), 
                            AsBoolean(opts["tie-param-lm"]));
    ilm.LoadLMs(lms);
    
    // Process features.
    vector<vector<string> > lmFeatures;
    if (opts["interpolation-features"]) {
        vector<string> features;
        trim_split(features, opts["interpolation-features"], ';');
        if (features.size() != 1 && features.size() != lms.size()) {
            Logger::Error(1, "# components specified in -interpolation-features"
                          " does not match number of LMs.\n");
            exit(1);
        }
        lmFeatures.resize(features.size());
        for (size_t l = 0; l < features.size(); ++l)
            trim_split(lmFeatures[l], features[l].c_str(), ',');
    }

    // Process interpolation.
    const char *interpolation = opts["interpolation"];
    Logger::Log(1, "Interpolation Method = %s\n", interpolation);
    if (strcmp(interpolation, "LI") == 0) {
        // Verify no features are specified.
        if (lmFeatures.size() > 0) {
            Logger::Error(1, "Linear interpolation uses no features.\n");
            exit(1);
        }
    } else {
        Interpolation mode;
        if (strcmp(interpolation, "CM") == 0) {
            if (lmFeatures.size() > 0) {
                for (size_t l = 0; l < lmFeatures.size(); ++l) {
                    if (lmFeatures[l].size() > 1) {
                        Logger::Error(1, "Too many features specified.\n");
                        exit(1);
                    }
                }
            } else {
                // Default CM features to log:sumhist:*.effcounts.
                lmFeatures.resize(1);
                lmFeatures[0].push_back("log:sumhist:%s.effcounts");
            }
            mode = CountMerging;
        } else if (strcmp(interpolation, "GLI") == 0) {
            mode = GeneralizedLinearInterpolation;
        } else {
            Logger::Error(1, "Unsupported interpolation mode %s.\n", 
                          interpolation);
            exit(1);
        }

        // Load features.
        vector<vector<FeatureVectors> > featureList(lms.size());
        for (size_t l = 0; l < lms.size(); ++l) {
            vector<string> &features = lmFeatures[lmFeatures.size()==1 ? 0 : l];
            featureList[l].resize(features.size());
            for (size_t f = 0; f < features.size(); ++f) {
                char feature[1024];
                sprintf(feature, features[f].c_str(), 
                        GetBasename(corpusFiles[l]).c_str());
                Logger::Log(1, "Loading feature for %s from %s...\n",
                            corpusFiles[l].c_str(), feature);
                ilm.model().LoadComputedFeatures(
                    featureList[l][f], feature, order);
            }
        }
        ilm.SetInterpolation(mode, featureList);
    }

    // Estimate LM.
    ParamVector params(ilm.defParams());
    if (opts["params"]) {
        Logger::Log(1, "Loading parameters from %s...\n", opts["params"]);
        ZFile f(opts["params"], "r");
        VerifyHeader(f, "Param");
        ReadVector(f, params);
        if (params.length() != ilm.defParams().length()) {
            Logger::Error(1, "Number of parameters mismatched.\n");
            exit(1);
        }
    }

    Optimization optAlg = ToOptimization(opts["opt-alg"]);
    if (optAlg == UnknownOptimization) {
        Logger::Error(1, "Unknown optimization algorithms '%s'.\n",
                      opts["opt-alg"]);
        exit(1);
    }
        
    if (opts["opt-perp"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development set %s...\n", opts["opt-perp"]);
            ZFile devZFile(opts["opt-perp"]);
            PerplexityOptimizer dev(ilm, order);
            dev.LoadCorpus(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optEntropy = dev.Optimize(params, optAlg);
            Logger::Log(2, " Best perplexity = %f\n", exp(optEntropy));
        }
    }
    if (opts["opt-margin"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development lattices %s...\n", 
                        opts["opt-margin"]);
            ZFile devZFile(opts["opt-margin"]);
            WordErrorRateOptimizer dev(ilm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optMargin = dev.OptimizeMargin(params, optAlg);
            Logger::Log(2, " Best margin = %f\n", optMargin);
        }
    }
    if (opts["opt-wer"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development lattices %s...\n", 
                        opts["opt-wer"]);
            ZFile devZFile(opts["opt-wer"]);
            WordErrorRateOptimizer dev(ilm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optWER = dev.OptimizeWER(params, optAlg);
            Logger::Log(2, " Best WER = %f%%\n", optWER);
        }
    }

    // Estimate full model.
    if (opts["write-lm"] || opts["eval-perp"] || 
        opts["eval-margin"] || opts["eval-wer"]) {
        Logger::Log(1, "Estimating full n-gram model...\n");
        ilm.Estimate(params);
    }

    // Save results.
    if (opts["write-params"]) {
        Logger::Log(1, "Saving parameters to %s...\n", opts["write-params"]);
        ZFile f(opts["write-params"], "w");
        WriteHeader(f, "Param");
        WriteVector(f, params);
    }
    if (opts["write-vocab"]) {
        Logger::Log(1, "Saving vocabulary to %s...\n", opts["write-vocab"]);
        ZFile vocabZFile(opts["write-vocab"], "w");
        ilm.SaveVocab(vocabZFile);
    }
    if (opts["write-lm"]) {
        Logger::Log(1, "Saving LM to %s...\n", opts["write-lm"]);
        ZFile lmZFile(opts["write-lm"], "w");
        ilm.SaveLM(lmZFile, writeBinary);
    }

    // Evaluate LM.
    if (opts["eval-perp"]) {
        Logger::Log(0, "Perplexity Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-perp"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            PerplexityOptimizer eval(ilm, order);
            eval.LoadCorpus(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputePerplexity(params));
        }
    }
    if (opts["eval-margin"]) {
        Logger::Log(0, "Margin Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-margin"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval lattices %s...\n", 
                        evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            WordErrorRateOptimizer eval(ilm, order);
            eval.LoadLattices(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputeMargin(params));
        }
    }
    if (opts["eval-wer"]) {
        Logger::Log(0, "WER Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-wer"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval lattices %s...\n", 
                        evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            WordErrorRateOptimizer eval(ilm, order);
            eval.LoadLattices(evalZFile);

            Logger::Log(0, "\t%s\t%.2f%%\n", evalFiles[i].c_str(),
                        eval.ComputeWER(params));
        }
    }

    return 0;
}
