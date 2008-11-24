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

#include "util/Logger.h"
#include "WordErrorRateOptimizer.h"

#define MAXLINE 4096

////////////////////////////////////////////////////////////////////////////////

WordErrorRateOptimizer::~WordErrorRateOptimizer() {
    for (size_t l = 0; l < _lattices.size(); ++l)
        delete _lattices[l];
}

void
WordErrorRateOptimizer::LoadLattices(const ZFile &latticesFile) {
    if (ReadUInt64(latticesFile) == MITLMv1) {
        _lattices.resize(ReadUInt64(latticesFile));
        for (size_t l = 0; l < _lattices.size(); ++l) {
            _lattices[l] = new Lattice(_lm);
            _lattices[l]->Deserialize(latticesFile);
        }
    } else {
        fseek(latticesFile, 0, SEEK_SET);
        char line[MAXLINE];
        while (getline(latticesFile, line, MAXLINE)) {
            // tag file trans
            if (line[0] == '#') continue;
            char *file = line;
            while (*file != 0 && !isspace(*file))  ++file;
            *file++ = '\0';
            char *trans = file;
            while (*trans != 0 && !isspace(*trans))  ++trans;
            *trans++ = '\0';
            while (*trans != 0 && isspace(*trans))  ++trans;
            Logger::Log(2, "Loading lattice %s...\n", line);
            Lattice *pLattice = new Lattice(_lm);
            pLattice->SetTag(line);
            pLattice->LoadLattice(ZFile(file, "r"));
            pLattice->SetReferenceText(trans);
            _lattices.push_back(pLattice);
        }
    }

    // Compute prob/bow masks.
    vector<BitVector> probMaskVectors(_order + 1);
    vector<BitVector> bowMaskVectors(_order);
    for (size_t o = 0; o <= _order; o++)
        probMaskVectors[o].reset(_lm.sizes(o), false);
    for (size_t o = 0; o < _order; o++)
        bowMaskVectors[o].reset(_lm.sizes(o), false);
    for (size_t l = 0; l < _lattices.size(); ++l) {
        const Lattice::ArcNgramIndexVector &arcProbs(_lattices[l]->_arcProbs);
        for (size_t i = 0; i < arcProbs.length(); ++i)
            probMaskVectors[arcProbs[i].order][arcProbs[i].ngramIndex] = true;

        const Lattice::ArcNgramIndexVector &arcBows(_lattices[l]->_arcBows);
        for (size_t i = 0; i < arcBows.length(); ++i) {
            bowMaskVectors[arcBows[i].order][arcBows[i].ngramIndex] = true;
        }
    }
    _mask = _lm.GetMask(probMaskVectors, bowMaskVectors);
}

void
WordErrorRateOptimizer::SaveLattices(const ZFile &latticesFile) {
    WriteUInt64(latticesFile, MITLMv1);
    WriteUInt64(latticesFile, _lattices.size());
    for (size_t l = 0; l < _lattices.size(); ++l)
        _lattices[l]->Serialize(latticesFile);
}

void
WordErrorRateOptimizer::SaveTranscript(const ZFile &transcriptFile) {
    string             line;
    vector<VocabIndex> bestPath;
    for (size_t l = 0; l < _lattices.size(); ++l) {
        const Lattice *lattice = _lattices[l];
        lattice->GetBestPath(bestPath);

        line = lattice->tag();
        for (size_t i = 0; i < bestPath.size(); ++i) {
            line += " ";
            line += _lm.vocab()[bestPath[i]];
        }

        fwrite(line.c_str(), sizeof(char), line.length(), transcriptFile);
        fputc('\n', transcriptFile);
    }
}

void
WordErrorRateOptimizer::SaveUttConfidence(const ZFile &confidenceFile) {
    for (size_t l = 0; l < _lattices.size(); ++l) {
        Lattice *lattice = _lattices[l];
        fprintf(confidenceFile, "%s\t%f\n",
                lattice->tag(), lattice->BuildConfusionNetwork());
    }
}

void
WordErrorRateOptimizer::SaveWER(const ZFile &werFile) {
    for (size_t l = 0; l < _lattices.size(); ++l) {
        Lattice *lattice = _lattices[l];
        fprintf(werFile, "%s\t%lu\t%i\n", lattice->tag(),
                lattice->refWords().length(), lattice->ComputeWER());
    }
}

double
WordErrorRateOptimizer::ComputeWER(const ParamVector &params) {
    // Estimate model.
    if (!_lm.Estimate(params, _mask))
        return 100;  // Out of bounds.

    size_t numErrors = 0;
    size_t totWords  = 0;
    for (size_t l = 0; l < _lattices.size(); ++l) {
        _lattices[l]->UpdateWeights();
        int wer = _lattices[l]->ComputeWER();
        numErrors += wer;
        totWords  += _lattices[l]->refWords().length();
    }
    double wer = (double)numErrors / totWords * 100;
    if (Logger::GetVerbosity() > 2) {
        Logger::Log(2, "%.2f%% = (%lu / %lu)\t", wer, numErrors, totWords);
        std::cout << params << std::endl;
    } else if (Logger::GetVerbosity() > 1)
        Logger::Log(2, "%.2f%% = (%lu / %lu)\n", wer, numErrors, totWords);
    return wer;
}

double
WordErrorRateOptimizer::ComputeOracleWER() const {
    size_t numErrors = 0;
    size_t totWords  = 0;
    for (size_t l = 0; l < _lattices.size(); ++l) {
        numErrors += _lattices[l]->oracleWER();
        totWords  += _lattices[l]->refWords().length();
    }
    return (double)numErrors / totWords * 100;
}

double
WordErrorRateOptimizer::ComputeMargin(const ParamVector &params) {
    // Estimate model.
    if (!_lm.Estimate(params, _mask))
        return _worstMargin - 10;  // Out of bounds.

    double totMargin = 0;
    for (size_t l = 0; l < _lattices.size(); ++l) {
        _lattices[l]->UpdateWeights();
        totMargin += _lattices[l]->ComputeMargin();
    }

    totMargin /= _lattices.size();
    if (Logger::GetVerbosity() > 2)
        std::cout << totMargin << "\t" << params << std::endl;
    else
        Logger::Log(2, "%f\n", totMargin);
    if (totMargin < _worstMargin)
        _worstMargin = totMargin;
    return totMargin;
}

double
WordErrorRateOptimizer::OptimizeMargin(ParamVector &params,
                                       Optimization technique) {
    _numCalls = 0;
    ComputeMarginFunc func(*this);
    int     numIter;
    double  minMargin;
    clock_t startTime = clock();
    switch (technique) {
    case PowellOptimization:
        minMargin = -MinimizePowell(func, params, numIter);
        break;
    case LBFGSOptimization:
        minMargin = -MinimizeLBFGS(func, params, numIter);
        break;
    case LBFGSBOptimization:
        minMargin = -MinimizeLBFGSB(func, params, numIter);
        break;
    default:
        throw std::runtime_error("Unsupported optimization technique.");
    }
    clock_t endTime = clock();

    Logger::Log(1, "Iterations   = %i\n", numIter);
    Logger::Log(1, "Elapsed Time = %f\n",
                float(endTime - startTime) / CLOCKS_PER_SEC);
    Logger::Log(1, "AvgMargin    = %f\n", minMargin);
    Logger::Log(1, "Func Evals   = %lu\n", _numCalls);
    Logger::Log(1, "OptParams    = [ ");
    for (size_t i = 0; i < params.length(); i++)
        Logger::Log(1, "%f ", params[i]);
    Logger::Log(1, "]\n");
    return minMargin;
}

double
WordErrorRateOptimizer::OptimizeWER(ParamVector &params,
                                    Optimization technique) {
    _numCalls = 0;
    ComputeWERFunc func(*this);
    int     numIter;
    double  minWER;
    clock_t startTime = clock();
    switch (technique) {
    case PowellOptimization:
        minWER = MinimizePowell(func, params, numIter);
        break;
    case LBFGSOptimization:
        minWER = MinimizeLBFGS(func, params, numIter);
        break;
    case LBFGSBOptimization:
        minWER = MinimizeLBFGSB(func, params, numIter);
        break;
    default:
        throw std::runtime_error("Unsupported optimization technique.");
    }
    clock_t endTime = clock();

    Logger::Log(1, "Iterations   = %i\n", numIter);
    Logger::Log(1, "Elapsed Time = %f\n",
                float(endTime - startTime) / CLOCKS_PER_SEC);
    Logger::Log(1, "WER          = %f%%\n", minWER);
    Logger::Log(1, "Func Evals   = %lu\n", _numCalls);
    Logger::Log(1, "OptParams    = [ ");
    for (size_t i = 0; i < params.length(); i++)
        Logger::Log(1, "%f ", params[i]);
    Logger::Log(1, "]\n");
    return minWER;
}
