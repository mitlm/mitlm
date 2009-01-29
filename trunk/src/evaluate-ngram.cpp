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
#include "util/Logger.h"
#include "util/ZFile.h"
#include "Types.h"
#include "Lattice.h"
#include "PerplexityOptimizer.h"
#include "WordErrorRateOptimizer.h"

using std::vector;
using std::string;

////////////////////////////////////////////////////////////////////////////////

const char *headerDesc =
"Usage: evaluate-ngram [Options]\n"
"\n"
"Evaluates the performance of an n-gram language model.  It also supports\n"
"various n-gram language model conversions, including changes in order,\n"
"vocabulary, and file format.\n"
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
    opts.AddOption("verbose", "Set verbosity level.", "1");
    opts.AddOption("o,order", "Set the n-gram order of the estimated LM.", "3");
    opts.AddOption("v,vocab", "Fix the vocab to only words from the specified file.");
    opts.AddOption("l,lm", "Load specified LM.");
    opts.AddOption("cl,compile-lattices", "[SLS] Compiles lattices into a binary format.");
    opts.AddOption("ep,eval-perp", "Compute test set perplexity.");
    opts.AddOption("ew,eval-wer", "Compute test set lattice word error rate.");
    opts.AddOption("em,eval-margin", "Compute test set lattice margin.");
    opts.AddOption("wb,write-binary", "Write LM/counts files in binary format.");
    opts.AddOption("wv,write-vocab", "Write LM vocab to file.");
    opts.AddOption("wl,write-lm", "Write n-gram LM to file.");
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
    if (!opts["lm"]) {
        Logger::Error(0, "Language model must be specified using -lm.");
        exit(1);
    }

    // Load language model.
    ArpaNgramLM lm(order);
    if (opts["vocab"]) {
        Logger::Log(1, "Loading vocab %s...\n", opts["vocab"]);
        ZFile vocabZFile(opts["vocab"]);
        lm.LoadVocab(vocabZFile);
    }
    Logger::Log(1, "Loading LM %s...\n", opts["lm"]);
    ZFile lmZFile(opts["lm"], "r");
    lm.LoadLM(lmZFile);

    // Compile lattices.
    if (opts["compile-lattices"]) {
        Logger::Log(0, "Compiling lattices %s:\n", opts["compile-lattices"]);
        ZFile latticesZFile(opts["compile-lattices"]);
        WordErrorRateOptimizer eval(lm, order);
        eval.LoadLattices(latticesZFile);
        string outFile(opts["compile-lattices"]);
        outFile += ".bin";
        ZFile outZFile(outFile.c_str(), "wb");
        eval.SaveLattices(outZFile);
    }

    // Evaluate LM.
    ParamVector params(lm.defParams());
    if (opts["eval-perp"]) {
        Logger::Log(0, "Perplexity Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-perp"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            PerplexityOptimizer eval(lm, order);
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
            WordErrorRateOptimizer eval(lm, order);
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
            WordErrorRateOptimizer eval(lm, order);
            eval.LoadLattices(evalZFile);

            Logger::Log(0, "\t%s\t%.2f%%\n", evalFiles[i].c_str(),
                        eval.ComputeWER(params));
        }
    }

    // Save results.
    if (opts["write-vocab"]) {
        Logger::Log(1, "Saving vocabulary to %s...\n", opts["write-vocab"]);
        ZFile vocabZFile(opts["write-vocab"], "w");
        lm.SaveVocab(vocabZFile);
    }
    if (opts["write-lm"]) {
        Logger::Log(1, "Saving LM to %s...\n", opts["write-lm"]);
        ZFile lmZFile(opts["write-lm"], "w");
        lm.SaveLM(lmZFile, writeBinary);
    }

    return 0;
}
