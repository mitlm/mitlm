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

#ifndef FASTIO_H
#define FASTIO_H

#include <stdint.h>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <vector>
#include "Logger.h"

////////////////////////////////////////////////////////////////////////////////

#define MITLMv1 0x20080901   // Use date as version ID.

////////////////////////////////////////////////////////////////////////////////

template <typename InputIterator, typename OutputIterator>
void Copy(InputIterator input, OutputIterator begin, const OutputIterator end) {
    while (begin != end) {
        *begin = *input;
        ++begin; ++input;
    }
}

inline void ReverseString(char *begin, char *end) {
    while(end > begin)
        std::swap(*begin++, *--end);
}

inline char *CopyString(char *dest, const char *src) {
    while (*src != 0)
        *dest++ = *src++;
    return dest;
}

inline char *CopyUInt(char *dest, unsigned int value) {
    char *p = dest;
    do {
        *p++ = '0' + (value % 10);
    } while (value /= 10);
    ReverseString(dest, p);
    return p;
}

inline char *CopyLProb(char *dest, double prob) {
    if (prob == 0) {
        *dest++ = '-';
        *dest++ = '9';
        *dest++ = '9';
        return dest;
    } else {
        double lprob = log10(prob);
        if (0 && lprob > -10.0) {
            int lprobInt = (int)(-lprob * 1000000);
            *dest++ = '-';
            char *p = dest;
            for (int i = 0; i < 6; i++) {
                *p++ = '0' + (lprobInt % 10);
                lprobInt /= 10;
            }
            *p++ = '.';
            *p++ = '0' + (lprobInt % 10);
            ReverseString(dest, p);
            return p;
        } else {
            return dest + sprintf(dest, "%.6f", lprob);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

inline bool getline(FILE *file, char *buf, size_t bufSize) {
    if (fgets(buf, bufSize, file)) {
        size_t len = strlen(buf) - 1;
        if (len >= bufSize)
            Logger::Error(1, "The following exceeded max length.\n%s\n", buf);
        else if (buf[len] == '\n')
            buf[len] = '\0';
        return true;
    } else
        return false;
}

inline bool getline(FILE *file, char *buf, size_t bufSize, size_t *outLen) {
    if (fgets(buf, bufSize, file)) {
        *outLen = strlen(buf) - 1;
        if (*outLen >= bufSize)
            Logger::Error(1, "The following exceeded max length.\n%s\n", buf);
        else if (buf[*outLen] == '\n')
            buf[*outLen] = '\0';
        return true;
    } else
        return false;
}

////////////////////////////////////////////////////////////////////////////////

inline void WriteAlignPad(FILE *outFile, size_t len) {
    uint64_t zero = 0;
    if ((len % 8) != 0 && fwrite(&zero, (8 - len) % 8, 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
}

inline void ReadAlignPad(FILE *inFile, size_t len) {
    uint64_t zero = 0;
    if ((len % 8) != 0 &&
        (fread(&zero, (8 - len) % 8, 1, inFile) != 1 || zero != 0))
        throw std::runtime_error("Read failed.");
}

inline void WriteInt32(FILE *outFile, int x) {
    if (fwrite(&x, sizeof(int), 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
}

inline void WriteUInt32(FILE *outFile, unsigned int x) {
    if (fwrite(&x, sizeof(unsigned int), 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
}

inline void WriteUInt64(FILE *outFile, uint64_t x) {
    if (fwrite(&x, sizeof(uint64_t), 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
}

inline void WriteDouble(FILE *outFile, double x) {
    if (fwrite(&x, sizeof(double), 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
}

inline void WriteString(FILE *outFile, const std::string &str) {
    WriteUInt64(outFile, (uint64_t)str.length());
    if (fwrite(str.c_str(), str.length(), 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
    WriteAlignPad(outFile, str.length());
}

template <typename T>
inline void WriteVector(FILE *out, const std::vector<T> &x) {
    WriteUInt64(out, (uint64_t)x.size());
    if (fwrite(x.data(), sizeof(T), x.size(), out) != x.size())
        throw std::runtime_error("Write failed.");
    WriteAlignPad(out, x.size() * sizeof(T));
}

inline void WriteHeader(FILE *outFile, const char *header) {
    size_t len = strlen(header);
    if (fwrite(header, len, 1, outFile) != 1)
        throw std::runtime_error("Write failed.");
    WriteAlignPad(outFile, len);
}

////////////////////////////////////////////////////////////////////////////////

inline int ReadInt32(FILE *inFile) {
    int v;
    if (fread(&v, sizeof(int), 1, inFile) != 1)
        throw std::runtime_error("Read failed.");
    return v;
}

inline unsigned int ReadUInt32(FILE *inFile) {
    unsigned int v;
    if (fread(&v, sizeof(unsigned int), 1, inFile) != 1)
        throw std::runtime_error("Read failed.");
    return v;
}

inline uint64_t ReadUInt64(FILE *inFile) {
    uint64_t v;
    if (fread(&v, sizeof(uint64_t), 1, inFile) != 1)
        throw std::runtime_error("Read failed.");
    return v;
}

inline double ReadDouble(FILE *inFile) {
    double v;
    if (fread(&v, sizeof(double), 1, inFile) != 1)
        throw std::runtime_error("Read failed.");
    return v;
}

inline void ReadString(FILE *inFile, std::string &str) {
    str.resize(ReadUInt64(inFile));
    if (fread(&str[0], str.length(), 1, inFile) != 1)
        throw std::runtime_error("Read failed.");
    ReadAlignPad(inFile, str.length());
}

template <typename T>
inline void ReadVector(FILE *in, std::vector<T> &x) {
    x.resize(ReadUInt64(in));
    if (fread(x.data(), sizeof(T), x.size(), in) != x.size())
        throw std::runtime_error("Read failed.");
    ReadAlignPad(in, x.size() * sizeof(T));
}

inline void VerifyHeader(FILE *inFile, const char *header) {
    char   buf[256];
    size_t len = strlen(header);
    assert(len < 255);
    if (fread(buf, len, 1, inFile) != 1 || strncmp(buf, header, len) != 0)
        throw std::runtime_error("Invalid file format.");
    ReadAlignPad(inFile, len);
}

#endif // FASTIO_H
