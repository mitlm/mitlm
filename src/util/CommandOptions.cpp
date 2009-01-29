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

#include <cassert>
#include <iostream>
#include "CommandOptions.h"
#include "util/Logger.h"

using namespace std;

CommandOptions::CommandOptions(const char *header, const char *footer) {
    _header = header;
    _footer = footer;
}

void
CommandOptions::AddOption(const char *name, 
                          const char *desc, 
                          const char *defval) {
    vector<string> names;
    trim_split(names, name, ',');
    for (size_t i = 0; i < names.size(); ++i)
        _nameIndexMap[names[i]] = _options.size();
    
    _options.push_back(CmdOption(name, desc, defval));
}

bool
CommandOptions::ParseArguments(int argc, const char **argv) {
    if (_header.length() == 0)
        _header = string("Usage: ") + argv[0] + " [Options]\n";

    _values.resize(_options.size(), NULL);
    int i = 1;
    while (i < argc) {
        const char *argName = argv[i++];
        assert(argName != NULL && argName[0] != '\0');
        hash_map_iter iter = _nameIndexMap.find(&argName[1]);
        if (iter == _nameIndexMap.end()) {
            cerr << "Invalid argument '" << argName << "'.\n";
            return false;
        }
        if (_values[iter->second] != NULL) {
            cerr << "Argument '" << argName << "' specified multiple times.\n";
            return false;
        }
        const char *value = "";
        if (i < argc && argv[i][0] != '-')
            value = argv[i++];
        _values[iter->second] = value;
    }

    for (size_t i = 0; i < _options.size(); ++i) {
        if (_values[i] == NULL)
            _values[i] = _options[i].defval;
//         Logger::Log(1, "%s = %s\n", 
//                     _options[i].name, (_values[i]==NULL ? "NULL" : _values[i]));
    }
    return true;
}

void
CommandOptions::PrintHelp() const {
    cout << _header << "\n";
    cout << "Options:\n";
    for (size_t i = 0; i < _options.size(); ++i) {
        vector<string> names;
        trim_split(names, _options[i].name, ',');
        cout << " -" << names[0];
        for (size_t j = 1; j < names.size(); j++)
            cout << ", -" << names[j];
        if (_options[i].defval == NULL) 
            cout << ":\n";
        else
            cout << " (" << _options[i].defval << "):\n";
        cout << "    " << _options[i].desc << "\n";
    }
    if (_footer.length() > 0 )
        cout << "\n" << _footer;
}
    
const char *
CommandOptions::operator[](const char *name) const {
    hash_map_iter iter = _nameIndexMap.find(name);
    assert(iter != _nameIndexMap.end());
    return _values[iter->second];
}

////////////////////////////////////////////////////////////////////////////////

vector<string> & 
trim_split(vector<string> &result, const char *str, char delimiter) {
    result.resize(0);
    if (str != NULL) {
        const char *end = str + strlen(str);
        while (str <= end) {
            while (isspace(*str)) ++str;
            const char *token = str;
            while (*str != delimiter && str < end) ++str;
            const char *tokenEnd = str++ - 1;
            while (isspace(*tokenEnd) && tokenEnd > token) --tokenEnd;
            result.push_back(string(token, tokenEnd - token + 1));
        }
    }
    return result;
}

