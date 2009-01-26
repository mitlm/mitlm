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
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "util/Logger.h"
#include "util/ZFile.h"
#include "Types.h"
#include "MaxLikelihoodSmoothing.h"
#include "KneserNeySmoothing.h"
#include "InterpolatedNgramLM.h"
#include "PerplexityOptimizer.h"
#include "WordErrorRateOptimizer.h"

using std::vector;
using std::string;
using std::cout;
using boost::split;
using boost::is_any_of;
namespace po = boost::program_options;
namespace cls = boost::program_options::command_line_style;

////////////////////////////////////////////////////////////////////////////////

const char *version = "0.1";
const char *progDesc =
"Usage: interpolate-ngram [Options] [lmfile1 lmfile2 ...]\n"
"\n"
"Takes multiple n-gram language models, computes appropriate interpolation\n"
"weights from optional features, and constructs a statically interpolated\n"
"n-gram model.  It also supports perplexity computation and parameter tuning\n"
"to optimize the development set perplexity.\n"
"\n"
"Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),\n"
"or '-' to indicate stdin/stdout.\n";

const char *footerDesc =
"Interpolation:\n"
"\n"
"MITLM currently supports the following interpolation algorithms:\n"
"\n"
" * LI - Interpolates the component models using linear interpolation.\n"
"   Constructs a model consisting of the union of all observed n-grams, with\n"
"   probabilities equal to the weighted sum of the component n-gram\n"
"   probabilities, using backoff, if necessary.  Backoff weights are computed\n"
"   to normalize the model.  In linear interpolation, the weight of each\n"
"   component is a fixed constant.\n"
" * CM - Interpolates the component models using count merging (Bacchiani\n"
"   et al., 2006; Hsu, 2007).  Unlike linear interpolation, the weight of each\n"
"   component depends on the n-gram history count for that component.\n"
" * GLI - Interpolates the component models using generalized linear\n"
"   interpolation (Hsu, 2007).  Unlike count merging, the weight of each\n"
"   component depends on arbitrary features of the n-gram history, specified\n"
"   through the --read-features option.\n"
"\n"
"References:\n"
"\n"
" * M. Bacchiani, M. Riley, B. Roark, and R. Sproat, \"MAP adaptation of\n"
"   stochastic grammars,\" Computer Speech & Language, vol. 20, no. 1,\n"
"   pp. 41-68, 2006.\n"
" * B. Hsu, \"Generalized linear interpolation of language models,\" \n"
"   in Proc. ASRU, 2007.\n"
" * F. Jelinek and R. Mercer, \"Interpolated estimation of Markov source\n"
"   parameters from sparse data,\" in Proc. Workshop on Pattern Recognition\n"
"   in Practice, 1980.\n";

////////////////////////////////////////////////////////////////////////////////

NgramLM *CreateNgramLM(string corpusFilename,
                       string vocabFilename,
                       size_t order,
                       string smoothingAlgs,
                       string weightFeatures,
                       bool useUnknown);
Smoothing *CreateSmoothing(const char *smoothing);


int main(int argc, char* argv[]) {
    // Parse command line options.
    int order, verbosity;
    po::options_description desc(string("Options"));
    desc.add_options()
        ("help", "Print this message.")
        ("version", "Print version number.")
        ("verbose", po::value<int>(&verbosity)->default_value(1),
         "Set verbosity level.")
        ("order,o", po::value<int>(&order)->default_value(3),
         "Set the n-gram order of the estimated LM.")
        ("read-vocab,v", po::value<string>(),
         "Restrict the vocabulary to only words from the specified vocabulary "
         "vocabfile.  n-grams with out of vocabulary words are ignored.")

        ("use-unknown,k", "Replace all words outside vocabulary with <unk>.")
        ("read-text,t", po::value<string>(),
         "Cumulate n-gram count statistics from textfile, where each line "
         "corresponds to a sentence.  Sentence boundary tags (<s>, </s>) are "
         "automatically added.  Empty lines are ignored.")
        ("read-count,c", po::value<string>(),
         "Read n-gram counts from countsfile.  Using both -read-text and "
         "-read-count will combine the counts.")
        ("read-wfeatures,w", po::value<string>()->default_value(""),
         "Read n-gram weighting features from feature file.")
        ("smoothing,s", po::value<string>()->default_value("ModKN"),
         "Apply specified smoothing algorithms to all n-gram orders.  See "
         "SMOOTHING.")
        ("read-lms,l", po::value<vector<string> >()->composing(),
         "Reads component LMs from specified lmfiles in either ARPA or binary "
         "format.")

        ("read-features,f", po::value<string>(),
         "Reads n-gram feature vectors from specified featurefiles, where each "
         "line contains an n-gram and its feature value.")
        ("tie-param-order", "Tie parameters across n-gram order.")
        ("tie-param-lm", "Tie parameters across LM components.")
        ("read-parameters,p", po::value<string>(),
         "Read model parameters from paramfile.")
        ("interpolation,i", po::value<string>()->default_value("LI"),
         "Apply specified interpolation algorithms to all n-gram orders.  "
         "See INTERPOLATION.")

        ("optimize-perplexity,d", po::value<string>(),
         "Tune the model parameters to minimize the perplexity of dev text.")
        ("optimize-margin,m", po::value<string>(),
         "Tune the model parameters to maximize the discriminative margin.")
        ("optimize-wer,a", po::value<string>(),
         "Tune the model parameters to minimize the word error rate.")

        ("evaluate-perplexity,e", po::value<vector<string> >()->composing(),
         "Compute the perplexity of textfile.  This option can be repeated.")
        ("write-vocab,V", po::value<string>(),
         "Write the LM vocabulary to vocabfile.")
        ("write-lm,L", po::value<string>(),
         "Write the interpolated n-gram language model to lmfile in ARPA "
         "backoff text format.")
        ("write-binary-lm,B", po::value<string>(),
         "Write  the interpolated n-gram language model to lmfile in MITLM "
         "binary format.")
        ("write-parameters,P", po::value<string>(),
         "Write model parameters to paramfile.")
        ;

    po::positional_options_description pd;
    pd.add("read-lms", -1);

    // Allow typical UNIX command line styles, except sticky (-s -k become -sk).
    // Add support for single dash long options (-help).
    cls::style_t options_style = (cls::style_t)(cls::allow_dash_for_short |
        cls::allow_short | cls::short_allow_adjacent | cls::short_allow_next |
        cls::allow_long | cls::long_allow_adjacent | cls::long_allow_next |
        cls::allow_long_disguise);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc)
              .style(options_style).positional(pd).run(), vm);
    //po::store(parse_config_file("example.cfg", desc), vm);
    po::notify(vm);

    // Process basic command line arguments.
    Logger::SetVerbosity(verbosity);
    if (vm.count("version")) {
        cout << "MIT Language Modeling Toolkit (v" << version << ")\n"
             << "Copyright (C) 2008 Bo-June (Paul) Hsu\n"
             << "MIT Computer Science and Artificial Intelligence Laboratory\n";
        return 0;
    }
    if (vm.count("help")) {
        cout << progDesc << "\n" << desc << "\n" << footerDesc << "\n";
        return 1;
    }

    // Read language models.
    vector<SharedPtr<NgramLMBase> > lms;
    vector<string> corpusFiles;
    string vocabFile = vm.count("read-vocab") ? vm["read-vocab"].as<string>() : "";
    if (vm.count("read-text")) {
        vector<string> sAlgs;
        vector<string> wFeats;
        split(corpusFiles, vm["read-text"].as<string>(), is_any_of(",;"));
        split(sAlgs, vm["smoothing"].as<string>(), is_any_of(";"));
        split(wFeats, vm["read-wfeatures"].as<string>(), is_any_of(";"));

        lms.resize(corpusFiles.size());
        for (size_t l = 0; l < corpusFiles.size(); ++l)
            lms[l] = CreateNgramLM(corpusFiles[l],
                                   vocabFile, order,
                                   sAlgs[sAlgs.size() == 1 ? 0 : l],
                                   wFeats.size() == 1 ? "" : wFeats[l],
                                   vm.count("use-unknown"));
    }

    // Read component language model input files.
    if (vm.count("read-lms")) {
        corpusFiles = vm["read-lms"].as<vector<string> >();
        for (size_t l = 0; l < corpusFiles.size(); l++) {
            Logger::Log(1, "Loading component LM %s...\n", corpusFiles[l].c_str());
            ArpaNgramLM *pLM = new ArpaNgramLM(order);
            if (vocabFile.length() > 0) {
                ZFile vocabZFile(vocabFile.c_str());
                pLM->LoadVocab(vocabZFile);
            }
            ZFile lmZFile(corpusFiles[l].c_str(), "r");
            pLM->LoadLM(lmZFile);
            lms.push_back((SharedPtr<NgramLMBase>)pLM);
        }
    }

    // Interpolate language models.
    Logger::Log(1, "Interpolating component LMs...\n");
    InterpolatedNgramLM ilm(order, vm.count("tie-param-order"), 
                            vm.count("tie-param-lm"));
    ilm.LoadLMs(lms);
    
    // Process features.
    string readFeatures;  // Declare here to allocate string buffer
    vector<vector<string> > lmFeatures(lms.size());
    if (vm.count("read-features")) {
        readFeatures = vm["read-features"].as<string>();
        vector<char *> features;
        char *p = &readFeatures[0];
        while (true) {
            char *token = p;
            while (*p != ';' && *p != '\0') ++p;
            if (*p == '\0') {
                features.push_back(token);
                break;
            } else {
                *p++ = 0;
                features.push_back(token);
            }
        }
        if (features.size() != lms.size()) {
            Logger::Error(1, "Number of components specified in read-features "
                             "does not match number of LMs.\n");
            exit(1);
        }
        for (size_t l = 0; l < lmFeatures.size(); ++l) {
            p = &features[l][0];
            while (*p != '\0') {
                char *token = p;
                while (*p != ',' && *p != '\0') ++p;
                if (*p != 0) *p++ = 0;
                lmFeatures[l].push_back(token);
            }
        }
    }

    // Process interpolation.
    string interpolation = vm["interpolation"].as<string>();
    Logger::Log(1, "Interpolation Method = %s\n", interpolation.c_str());
    if (interpolation == "LI") {
        // Verify no features are specified.
        for (size_t l = 0; l < lmFeatures.size(); ++l)
            if (lmFeatures[l].size() > 0) {
                Logger::Error(1, "Linear interpolation uses no features.\n");
                exit(1);
            }
    } else {
        Interpolation mode;
        if (interpolation == "CM") {
            // Default CM features to log:sumhist:*.counts.
            for (size_t l = 0; l < lms.size(); ++l) {
                if (lmFeatures[l].size() > 1) {
                    Logger::Error(1, "Too many count features specified.\n");
                    exit(1);
                } else if (lmFeatures[l].size() == 0) {
                    size_t extIndex = corpusFiles[l].find_last_of('.');
                    lmFeatures[l].push_back("log:sumhist:" +
                        corpusFiles[l].substr(0, extIndex) + ".counts");
                }
            }
            mode = CountMerging;
        } else if (interpolation == "GLI") {
            mode = GeneralizedLinearInterpolation;
        } else {
            Logger::Error(1, "Unsupported interpolation mode %s.\n",
                          interpolation.c_str());
            exit(1);
        }

        // Load features.
        vector<vector<FeatureVectors> > featureList(lms.size());
        for (size_t l = 0; l < lms.size(); ++l) {
            featureList[l].resize(lmFeatures[l].size());
            for (size_t f = 0; f < lmFeatures[l].size(); ++f) {
                Logger::Log(1, "Loading feature for %s from %s...\n",
                            corpusFiles[l].c_str(), lmFeatures[l][f].c_str());
                ilm.model().LoadComputedFeatures(
                    featureList[l][f], lmFeatures[l][f].c_str(), ilm.order());
                if (featureList[l][f][0].length() == 0)
                    Logger::Warn(1, "0th order should contain total count.\n");
            }
        }
        ilm.SetInterpolation(mode, featureList);
    }

    // Estimate LM.
    ParamVector params(ilm.defParams());
    if (vm.count("read-parameters")) {
        const char *paramFile = vm["read-parameters"].as<string>().c_str();
        Logger::Log(1, "Loading parameters from %s...\n", paramFile);
        ZFile f(paramFile, "rb");
        VerifyHeader(f, "Param");
        ReadVector(f, params);
        if (params.length() != ilm.defParams().length()) {
            Logger::Error(1, "Number of parameters mismatched\n.");
            exit(1);
        }
    }
    if (vm.count("optimize-perplexity")) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            const char *devFile=vm["optimize-perplexity"].as<string>().c_str();

            Logger::Log(1, "Loading development set %s...\n", devFile);
            ZFile devZFile(devFile);
            PerplexityOptimizer dev(ilm, order);
            dev.LoadCorpus(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
//            double optEntropy = dev.Optimize(params, LBFGSOptimization);
            double optEntropy = dev.Optimize(params, PowellOptimization);
            Logger::Log(2, " Best perplexity = %f\n", exp(optEntropy));
        }
    }
    if (vm.count("optimize-margin")) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            const char *devFile=vm["optimize-margin"].as<string>().c_str();

            Logger::Log(1, "Loading development set %s...\n", devFile);
            ZFile devZFile(devFile);
            WordErrorRateOptimizer dev(ilm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optMargin = dev.OptimizeMargin(params, PowellOptimization);
            Logger::Log(2, " Best margin = %f\n", optMargin);
        }
    }
    if (vm.count("optimize-wer")) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            const char *devFile=vm["optimize-wer"].as<string>().c_str();

            Logger::Log(1, "Loading development set %s...\n", devFile);
            ZFile devZFile(devFile);
            WordErrorRateOptimizer dev(ilm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optWER = dev.OptimizeWER(params, PowellOptimization);
            Logger::Log(2, " Best WER = %f%%\n", optWER);
        }
    }

    // Evaluate LM.
    if (vm.count("evaluate-perplexity")) {
        const vector<string> &evalFiles = vm["evaluate-perplexity"]
            .as<vector<string> >();

        Logger::Log(0, "Perplexity Evaluations:\n");
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            PerplexityOptimizer eval(ilm, order);
            eval.LoadCorpus(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputePerplexity(params));
        }
    }

    Logger::Log(1, "Estimating full n-gram model...\n");
    ilm.Estimate(params);

    // Save results.
    if (vm.count("write-vocab")) {
        const char *vocabFile = vm["write-vocab"].as<string>().c_str();
        Logger::Log(1, "Saving vocabulary to %s...\n", vocabFile);
        ZFile vocabZFile(vocabFile, "w");
        ilm.SaveVocab(vocabZFile);
    }
    if (vm.count("write-lm")) {
        const char *lmFile = vm["write-lm"].as<string>().c_str();
        Logger::Log(1, "Saving LM to %s...\n", lmFile);
        ZFile lmZFile(lmFile, "w");
        ilm.SaveLM(lmZFile);
    }
    if (vm.count("write-binary-lm")) {
        const char *lmFile = vm["write-binary-lm"].as<string>().c_str();
        Logger::Log(1, "Saving binary LM to %s...\n", lmFile);
        ZFile lmZFile(lmFile, "wb");
        ilm.SaveLM(lmZFile, true);
    }
    if (vm.count("write-parameters")) {
        const char *paramFile = vm["write-parameters"].as<string>().c_str();
        Logger::Log(1, "Saving parameters to %s...\n", paramFile);
        ZFile f(paramFile, "wb");
        WriteHeader(f, "Param");
        WriteVector(f, params);
    }

    return 0;
}

NgramLM *CreateNgramLM(string corpusFilename,
                       string vocabFilename,
                       size_t order,
                       string smoothingAlgs,
                       string weightFeatures,
                       bool useUnknown) {
    NgramLM *pLM = new NgramLM(order);
    if (useUnknown) {
        Logger::Log(1, "Replace unknown words with <unk>...\n");
        pLM->UseUnknown();
    }
    if (vocabFilename.length() != 0) {
        Logger::Log(1, "Loading vocab %s...\n", vocabFilename.c_str());
        ZFile vocabZFile(vocabFilename.c_str());
        pLM->LoadVocab(vocabZFile);
    }
    Logger::Log(1, "Loading corpus %s...\n", corpusFilename.c_str());
    ZFile corpusZFile(ZFile(corpusFilename.c_str()));
    pLM->LoadCorpus(corpusZFile);
    
    // Process n-gram weighting features.
    if (weightFeatures.size() > 0) {
        vector<string> wFeats;
        split(wFeats, weightFeatures, is_any_of(","));
        vector<vector<DoubleVector> > featureList(wFeats.size());
        for (size_t f = 0; f < wFeats.size(); ++f) {
            const char *featFilename = wFeats[f].c_str();
            Logger::Log(1, "Loading weight features %s...\n", featFilename);
            pLM->model().LoadComputedFeatures(featureList[f], featFilename);
        }
        pLM->SetWeighting(featureList);
    }
    
    // Set smoothing algorithms.
    Logger::Log(1, "Set smoothing algorithms...\n");
    vector<SharedPtr<Smoothing> > smoothings(order + 1);
    for (size_t o = 1; o <= order; o++) {
        Logger::Log(1, "Smoothing[%i] = %s\n", o, smoothingAlgs.c_str());
        smoothings[o] = CreateSmoothing(smoothingAlgs.c_str());
        if (smoothings[o].get() == NULL) {
            Logger::Error(1, "Unknown smoothing %s.\n", smoothingAlgs.c_str());
            exit(1);
        }
    }
    pLM->SetSmoothingAlgs(smoothings);
    return pLM;
}

Smoothing *CreateSmoothing(const char *smoothing) {
    if (strcmp(smoothing, "FixKN") == 0) {
        return new KneserNeySmoothing(1, false);
    } else if (strcmp(smoothing, "FixModKN") == 0) {
        return new KneserNeySmoothing(3, false);
    } else if (strncmp(smoothing, "FixKN", 5) == 0) {
        for (size_t i = 5; i < strlen(smoothing); ++i)
            if (!isdigit(smoothing[i])) return NULL;
        return new KneserNeySmoothing(atoi(&smoothing[5]), false);
    } else if (strcmp(smoothing, "KN") == 0) {
        return new KneserNeySmoothing(1, true);
    } else if (strcmp(smoothing, "ModKN") == 0) {
        return new KneserNeySmoothing(3, true);
    } else if (strncmp(smoothing, "KN", 2) == 0) {
        for (size_t i = 2; i < strlen(smoothing); ++i)
            if (!isdigit(smoothing[i])) return NULL;
        return new KneserNeySmoothing(atoi(&smoothing[2]), true);
    } else if (strcmp(smoothing, "ML") == 0) {
        return new MaxLikelihoodSmoothing();
    } else
        return NULL;
}
