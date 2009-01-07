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
#include "util/ZFile.h"
#include "util/Logger.h"
#include "Types.h"
#include "NgramLM.h"
#include "MaxLikelihoodSmoothing.h"
#include "KneserNeySmoothing.h"
#include "PerplexityOptimizer.h"
#include "WordErrorRateOptimizer.h"

using std::vector;
using std::string;
using std::cout;
namespace po = boost::program_options;
namespace cls = boost::program_options::command_line_style;

////////////////////////////////////////////////////////////////////////////////

const char *version = "0.1";
const char *progDesc =
"Usage: estimate-ngram [Options] [textfile lmfile]\n"
"\n"
"Estimates an n-gram language model by cumulating n-gram count statistics from a\n"
"raw text file, applying smoothing to distribute counts from observed n-grams to\n"
"unseen ones, and building a backoff n-gram language model from counts.  It also\n"
"supports perplexity computation and parameter tuning to optimize the development\n"
"set perplexity.\n"
"\n"
"Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),\n"
"or '-' to indicate stdin/stdout.\n";

const char *footerDesc =
"Smoothing:\n"
"\n"
"MITLM currently supports the following smoothing algorithms:\n"
" * ML - Maximum likelihood estimation.\n"
" * KN - Original interpolated Kneser-Ney smoothing with default parameters\n"
"   estimated from count of count statistics (Kneser and Ney, 95).\n"
" * ModKN - Modified interpolated Kneser-Ney smoothing with default parameters\n"
"   estimated from count of count statistics (Chen and Goodman, 1998).\n"
" * KNd - Extended interpolated Kneser-Ney smoothing with d discount parameters\n"
"   per n-gram order.  The default parameters are estimated from count of count\n"
"   statistics.  KN1 and KN3 are equivalent to KN and ModKN, respectively.\n"
" * FixKN - KN using parameters estimated from count statistics.\n"
" * FixModKN - ModKN using parameters estimated from count statistics.\n"
" * FixKNd - KNd using parameters estimated from count statistics.\n"
"\n"
"Examples:\n"
"\n"
"Construct a standard trigram model from a text file using modified Kneser-Ney\n"
"smoothing with default parameters.\n"
"   estimate-ngram --read-text input.txt --smoothing ModKN --write-lm output.lm\n"
"\n"
"Build a 4-gram model from pre-computed counts using modified Kneser-Ney\n"
"smoothing, with discount parameters tuned on a development set.\n"
"   estimate-ngram --order 4 --read-count input.count --smoothing ModKN \\\n"
"    --optimize-perplexity dev.txt --write-lm output.lm\n"
"\n"
"Build a trigram model from text using modified Kneser-Ney smoothing with\n"
"specific parameters.\n"
"   estimate-ngram --read-text input.txt --smoothing ModKN \\\n"
"    --read-parameters input.param --write-lm output.lm\n"
"\n"
"References:\n"
"\n"
" * S. Chen and J. Goodman, \"An empirical study of smoothing techniques for\n"
"   language modeling,\" in Technical Report TR-10-98. Computer Science Group,\n"
"   Harvard University, 1998.\n"
" * R. Kneser and H. Ney, \"Improved backing-off for m-gram language modeling,\"\n"
"   in Proc. ICASSP, 1995.\n";

////////////////////////////////////////////////////////////////////////////////

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
        ("read-text,t", po::value<vector<string> >()->composing(),
         "Cumulate n-gram count statistics from textfile, where each line "
         "corresponds to a sentence.  Sentence boundary tags (<s>, </s>) are "
         "automatically added.  Empty lines are ignored.")
        ("read-count,c", po::value<vector<string> >()->composing(),
         "Read n-gram counts from countsfile.  Using both -read-text and "
         "-read-count will combine the counts.")
        ("read-wfeatures,w", po::value<vector<string> >()->composing(),
         "Read n-gram weighting features from feature file.")
        ("read-parameters,p", po::value<string>(),
         "Read model parameters from paramfile.")
        ("smoothing,s", po::value<string>()->default_value("ModKN"),
         "Apply specified smoothing algorithms to all n-gram orders.  See "
         "SMOOTHING.")
        ("smoothingN,N", po::value<string>(),
         "Apply specified smoothing algorithms to n-gram order N.  "
         "Ex. --smoothing1 KN -2 WB.  See SMOOTHING.")
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
        ("write-count,C", po::value<string>(),
         "Write n-gram counts to countsfile.")
        ("write-binary-count,D", po::value<string>(),
         "Write n-gram counts to countsfile in MITLM binary format.")
        ("write-eff-count,E", po::value<string>(),
         "Write effective n-gram counts to countsfile.")
        ("write-lm,L", po::value<string>(),
         "Write n-gram language model to lmfile in ARPA backoff text format.")
        ("write-binary-lm,B", po::value<string>(),
         "Write n-gram language model to lmfile in MITLM binary format.")
        ("write-parameters,P", po::value<string>(),
         "Write model parameters to paramfile.")
        ;

    po::positional_options_description pd;
    pd.add("read-text", 1);
    pd.add("write-lm", 1);

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
    if (vm.count("help") ||
        (vm.count("read-text") == 0 && vm.count("read-count") == 0)) {
        cout << progDesc << "\n" << desc << "\n" << footerDesc << "\n";
        return 1;
    }

    // Read language model input files.
    NgramLM lm(order);
    if (vm.count("use-unknown")) {
        Logger::Log(1, "Replace unknown words with <unk>...\n");
        lm.UseUnknown();
    }
    if (vm.count("read-vocab")) {
        const char *vocabFile = vm["read-vocab"].as<string>().c_str();
        Logger::Log(1, "Loading vocab %s...\n", vocabFile);
        ZFile vocabZFile(vocabFile);
        lm.LoadVocab(vocabZFile);
    }
    if (vm.count("read-text")) {
        const vector<string> &corpusFiles = vm["read-text"]
            .as<vector<string> >();
        for (size_t i = 0; i < corpusFiles.size(); i++) {
            Logger::Log(1, "Loading corpus %s...\n", corpusFiles[i].c_str());
            ZFile corpusZFile(ZFile(corpusFiles[i].c_str()));
            lm.LoadCorpus(corpusZFile);
        }
    }
    if (vm.count("read-count")) {
        const vector<string> &countFiles = vm["read-count"]
            .as<vector<string> >();
        for (size_t i = 0; i < countFiles.size(); i++) {
            Logger::Log(1, "Loading counts %s...\n", countFiles[i].c_str());
            ZFile countZFile(ZFile(countFiles[i].c_str()));
            lm.LoadCounts(countZFile);
        }
    }

    // Process n-gram weighting features.
    if (vm.count("read-wfeatures")) {
        vector<string> readWFeats = vm["read-wfeatures"].as<vector<string> >();
        vector<vector<DoubleVector> > featureList(readWFeats.size());
        for (size_t f = 0; f < readWFeats.size(); ++f) {
            const char *featFilename = readWFeats[f].c_str();
            Logger::Log(1, "Loading weight features %s...\n", featFilename);
            lm.model().LoadComputedFeatures(featureList[f], featFilename);
        }
        lm.SetWeighting(featureList);
    }

    // Set smoothing algorithms.
    assert(vm.count("smoothing"));
    const char *defSmoothing = vm["smoothing"].as<string>().c_str();
    vector<SharedPtr<Smoothing> > smoothings(order + 1);
    for (int o = 1; o <= order; o++) {
        string optionName = "smoothing";  optionName += ('0' + o);
        const char *smoothing = vm.count(optionName) ?
            vm[optionName].as<string>().c_str() : defSmoothing;
        Logger::Log(1, "Smoothing[%i] = %s\n", o, smoothing);
        smoothings[o] = CreateSmoothing(smoothing);
        if (smoothings[o].get() == NULL) {
            Logger::Error(1, "Unknown smoothing %s.\n", smoothing);
            exit(1);
        }
    }
    Logger::Log(1, "Set smoothing algorithms...\n");
    lm.SetSmoothingAlgs(smoothings);

    // Estimate LM.
    ParamVector params(lm.defParams());
    if (vm.count("read-parameters")) {
        const char *paramFile = vm["read-parameters"].as<string>().c_str();
        Logger::Log(1, "Loading parameters from %s...\n", paramFile);
        ZFile f(paramFile, "rb");
        VerifyHeader(f, "Param");
        ReadVector(f, params);
        if (params.length() != lm.defParams().length()) {
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
            PerplexityOptimizer dev(lm, order);
            dev.LoadCorpus(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
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
            WordErrorRateOptimizer dev(lm, order);
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
            WordErrorRateOptimizer dev(lm, order);
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
            PerplexityOptimizer eval(lm, order);
            eval.LoadCorpus(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputePerplexity(params));
        }
    }

    Logger::Log(1, "Estimating full n-gram model...\n");
    lm.Estimate(params);

    // Save results.
    if (vm.count("write-vocab")) {
        const char *vocabFile = vm["write-vocab"].as<string>().c_str();
        Logger::Log(1, "Saving vocabulary to %s...\n", vocabFile);
        ZFile vocabZFile(vocabFile, "w");
        lm.SaveVocab(vocabZFile);
    }
    if (vm.count("write-count")) {
        const char *countFile = vm["write-count"].as<string>().c_str();
        Logger::Log(1, "Saving counts to %s...\n", countFile);
        ZFile countZFile(countFile, "w");
        lm.SaveCounts(countZFile);
    }
    if (vm.count("write-binary-count")) {
        const char *countFile = vm["write-binary-count"].as<string>().c_str();
        Logger::Log(1, "Saving binary counts to %s...\n", countFile);
        ZFile countZFile(countFile, "wb");
        lm.SaveCounts(countZFile, true);
    }
    if (vm.count("write-eff-count")) {
        const char *countFile = vm["write-eff-count"].as<string>().c_str();
        Logger::Log(1, "Saving effective counts to %s...\n", countFile);
        ZFile countZFile(countFile, "w");
        lm.SaveEffCounts(countZFile);
    }
    if (vm.count("write-lm")) {
        const char *lmFile = vm["write-lm"].as<string>().c_str();
        Logger::Log(1, "Saving LM to %s...\n", lmFile);
        ZFile lmZFile(lmFile, "w");
        lm.SaveLM(lmZFile);
    }
    if (vm.count("write-binary-lm")) {
        const char *lmFile = vm["write-binary-lm"].as<string>().c_str();
        Logger::Log(1, "Saving binary LM to %s...\n", lmFile);
        ZFile lmZFile(lmFile, "wb");
        lm.SaveLM(lmZFile, true);
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
