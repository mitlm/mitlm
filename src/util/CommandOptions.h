////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// Copyright (c) 2013, Giulio Paci <giuliopaci@gmail.com>                 //
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

#include <cstdlib>
#include <vector>
#include <string>
#include <tr1/unordered_map>

using std::string;
using std::vector;

namespace mitlm {

class CommandOptions {
protected:
    struct CmdOption {
        CmdOption(const char *name_, const char *desc_, const char *defval_, const char *type_)
            : name(name_), desc(desc_), defval(defval_), type(type_) { }
        const char *name;
        const char *desc;
        const char *defval;
        const char *type;
    };

    typedef std::tr1::unordered_map<string, int>::const_iterator hash_map_iter;

    string                _header;
    string                _footer;
    vector<CmdOption>     _options;
    vector<const char *>  _values;
    std::tr1::unordered_map<string, int> _nameIndexMap;

public:
    CommandOptions(const char *header="", const char *footer="");
    void AddOption(const char *name, const char *desc, const char *defval=NULL, const char *type=NULL);
    bool ParseArguments(int argc, const char **argv);
    void PrintHelp() const;
    
    const char * operator[](const char *name) const;
};

vector<string> & 
trim_split(vector<string> &result, const char *str, char delimiter);

inline const char *GetItem(vector<string> &items, size_t index) {
    if (items.size() == 0) return NULL;
    if (items.size() == 1) return items[0].c_str();
    return items[index].c_str();
}

inline string GetBasename(string str) {
    size_t extIndex = str.find_last_of('.');
    return extIndex == string::npos ? str : str.substr(0, extIndex);
}

inline bool AsBoolean(const char *str) {
    if (str == NULL) return false;
    if (str[0] == 't' || str[0] == 'T' || str[0] == '1') return true;
    return false;
}

}
