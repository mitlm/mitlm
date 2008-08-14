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
#include "util/Logger.h"
#include "util/ZFile.h"
#include "Types.h"
#include "InterpolatedNgramLM.h"
#include "NgramPerplexity.h"

using std::vector;
using std::string;
using std::cout;
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

void LoadComputedFeatures(const NgramModel &model,
                          vector<DoubleVector> &feature,
                          string &featFilename);

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
        ("read-lms,l", po::value<vector<string> >()->composing(),
         "Reads component LMs from specified lmfiles in either ARPA or binary "
         "format.")
        ("read-features,f", po::value<vector<string> >()->composing(),
         "Reads n-gram feature vectors from specified featurefiles, where each "
         "line contains an n-gram and its feature value.  Since interpolation "
         "weights are computed on features of the n-gram history, it is "
         "sufficient to have features only up to order n - 1.")
        ("read-parameters,p", po::value<string>(),
         "Read model parameters from paramfile.")
        ("interpolation,i", po::value<string>()->default_value("LI"),
         "Apply specified interpolation algorithms to all n-gram orders.  "
         "See INTERPOLATION.")
        ("optimize-perplexity,d", po::value<string>(),
         "Tune the model parameters to minimize the perplexity of dev text.")
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
    if (vm.count("help") || vm.count("read-lms") == 0) {
        cout << progDesc << "\n" << desc << "\n" << footerDesc << "\n";
        return 1;
    }

    const char *vocabFile = NULL;
    if (vm.count("read-vocab")) {
        vocabFile = vm["read-vocab"].as<string>().c_str();
        Logger::Log(1, "Loading vocab %s...\n", vocabFile);
    }

    // Read component language model input files.
    const vector<string> &readLMs = vm["read-lms"].as<vector<string> >();
    vector<SharedPtr<NgramLMBase> > lms(readLMs.size());
    for (size_t l = 0; l < readLMs.size(); l++) {
        Logger::Log(1, "Loading component LM %s...\n", readLMs[l].c_str());
        ArpaNgramLM *pLM = new ArpaNgramLM();
        pLM->SetOrder(order);
        if (vocabFile)
            pLM->LoadVocab(ZFile(vocabFile));
        pLM->LoadLM(ZFile(readLMs[l].c_str(), "r"));
        lms[l] = pLM;
    }
    Logger::Log(1, "Interpolating component LMs...\n");
    InterpolatedNgramLM ilm;
    ilm.SetOrder(order);
    ilm.LoadLMs(lms);

    // Process interpolation & features.
    string         interpolation = vm["interpolation"].as<string>();
    vector<string> readFeatures;
    if (vm.count("read-features"))
        readFeatures = vm["read-features"].as<vector<string> >();
    if (interpolation == "LI") {
        // No features.
        if (readFeatures.size()) {
            Logger::Warn(1, "Linear interpolation does not utilize features.\n");
            Logger::Warn(1, "%lu features ignored.\n", readFeatures.size());
        }
    } else if (interpolation == "CM") {
        // Use features specified by --read-features or default .counts files.
        if (readFeatures.size() > readLMs.size()) {
            Logger::Warn(1, "Too many count features specified.\n");
            Logger::Warn(1, "%lu features ignored.\n",
                        readFeatures.size() - readLMs.size());
        }
        vector<vector<DoubleVector> > featureList(readLMs.size());
        size_t numReadFeatures = readFeatures.size();
        readFeatures.resize(readLMs.size());
        for (size_t f = 0; f < readLMs.size(); ++f) {
            if (f >= numReadFeatures) {
                size_t extIndex = readLMs[f].find_last_of('.');
                readFeatures[f] = "log:" + readLMs[f].substr(0, extIndex) +
                                  ".counts";
            }
            Logger::Log(1, "Loading counts for %s from %s...\n",
                       readLMs[f].c_str(), readFeatures[f].c_str());
            LoadComputedFeatures(ilm.model(), featureList[f], readFeatures[f]);
        }
        ilm.SetInterpolation(CM, featureList);
    } else if (interpolation == "GLI") {
        vector<vector<DoubleVector> > featureList(readFeatures.size());
        for (size_t f = 0; f < readFeatures.size(); ++f) {
            Logger::Log(1, "Loading features %s...\n", readFeatures[f].c_str());
            LoadComputedFeatures(ilm.model(), featureList[f], readFeatures[f]);
        }
        ilm.SetInterpolation(GLI, featureList);
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
            NgramPerplexity dev(ilm);
            dev.LoadCorpus(ZFile(devFile));

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optEntropy = dev.Optimize(params, LBFGSBOptimization);
//            double optEntropy = dev.Optimize(params, PowellOptimization);
            Logger::Log(2, " Best perplexity = %f\n", exp(optEntropy));
        }
    }
    Logger::Log(1, "Estimating full n-gram model...\n");
    ilm.Estimate(params);

    // Evaluate LM.
    if (vm.count("evaluate-perplexity")) {
        const vector<string> &evalFiles = vm["evaluate-perplexity"]
            .as<vector<string> >();

        Logger::Log(0, "Perplexity Evaluations:\n");
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            NgramPerplexity eval(ilm);
            eval.LoadCorpus(ZFile(evalFiles[i].c_str()));

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                       eval.ComputePerplexity(params));
        }
    }

    // Save results.
    if (vm.count("write-vocab")) {
        const char *vocabFile = vm["write-vocab"].as<string>().c_str();
        Logger::Log(1, "Saving vocabulary to %s...\n", vocabFile);
        ilm.SaveVocab(ZFile(vocabFile, "w"));
    }
    if (vm.count("write-lm")) {
        const char *lmFile = vm["write-lm"].as<string>().c_str();
        Logger::Log(1, "Saving LM to %s...\n", lmFile);
        ilm.SaveLM(ZFile(lmFile, "w"));
    }
    if (vm.count("write-binary-lm")) {
        const char *lmFile = vm["write-binary-lm"].as<string>().c_str();
        Logger::Log(1, "Saving binary LM to %s...\n", lmFile);
        ilm.SaveLM(ZFile(lmFile, "wb"), true);
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

void LoadComputedFeatures(const NgramModel &model,
                          vector<DoubleVector> &feature,
                          string &featFilename) {
    size_t colonIndex = featFilename.find(':');
    const char *featFunc, *filename;
    if (colonIndex == string::npos) {
        featFunc = NULL;
        filename = featFilename.c_str();
    } else {
        featFilename[colonIndex] = 0;
        featFunc = featFilename.c_str();
        filename = &featFilename[colonIndex + 1];
    }

    ZFile f(filename, "r");
    model.LoadFeatures(feature, f, model.size() - 1);
    if (featFunc != NULL) {
        if (strcmp(featFunc, "log") == 0)
            for (size_t o = 0; o < feature.size(); ++o)
                feature[o] = log(feature[o] + 1e-99);
        else if (strcmp(featFunc, "log1p") == 0)
            for (size_t o = 0; o < feature.size(); ++o)
                feature[o] = log(feature[o] + 1);
        else if (strcmp(featFunc, "pow2") == 0)
            for (size_t o = 0; o < feature.size(); ++o)
                feature[o] *= feature[o];
        else if (strcmp(featFunc, "pow3") == 0)
            for (size_t o = 0; o < feature.size(); ++o)
                feature[o] = pow(feature[o], 3);
        else {
            Logger::Error(1, "Unknown feature function: %s\n", featFunc);
            exit(1);
        }
    }

    for (size_t o = 0; o < feature.size(); ++o) {
        if (anyTrue(feature[o] > 20.0)) {
            Logger::Warn(1, "Feature value %s exceed 20.0.\n", filename);
            break;
        }
    }
}
