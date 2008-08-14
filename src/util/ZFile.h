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

#ifndef ZFILE_H
#define ZFILE_H

#include <cstdio>
#include <string>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////

class ZFile {
protected:
    FILE *_file;

    bool endsWith(const char *str, const char *suffix) {
        size_t strLen = strlen(str);
        size_t suffixLen = strlen(suffix);
        return (suffixLen <= strLen) &&
               (strncmp(&str[strLen - suffixLen], suffix, suffixLen) == 0);
    }

    FILE *processOpen(const std::string &command, const char *mode)
    { return popen(command.c_str(), mode); }

public:
    ZFile(const char *filename, const char *mode="rb") {
        if (mode == NULL || (mode[0] != 'r' && mode[0] != 'w'))
            throw std::runtime_error("Invalid mode");

        if (endsWith(filename, ".gz")) {
            _file = (mode[0] == 'r') ?
                processOpen(std::string("exec gunzip -c ") + filename, "r") :
                processOpen(std::string("exec gzip -c > ") + filename, "w");
        } else if (endsWith(filename, ".bz2")) {
            _file = (mode[0] == 'r') ?
                processOpen(std::string("exec bunzip2 -c ") + filename, "r") :
                processOpen(std::string("exec bzip2 > ") + filename, "w");
        } else if (endsWith(filename, ".zip")) {
            _file = (mode[0] == 'r') ?
                processOpen(std::string("exec unzip -c ") + filename, "r") :
                processOpen(std::string("exec zip -q > ") + filename, "w");
        } else { // Assume uncompressed
            _file = fopen(filename, mode);
        }
        if (_file == NULL)
            throw std::runtime_error("Cannot open file");
    }
    ~ZFile() { if (_file) fclose(_file); }

    operator FILE *() const { return _file; }
};

#endif // ZFILE_H
