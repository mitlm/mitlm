////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// Copyright (c) 2010-2013, Giulio Paci <giuliopaci@gmail.com>            //
// Copyright (c) 2013, Jakub Wilk  <jwilk@debian.org>                     //
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

#include <fcntl.h>
#include <cstdio>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sstream>

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace mitlm {

#if ( defined(WIN32) || defined(_WIN32) ) && !defined(__CYGWIN__)

static std::string win_argv_escape ( const std::string& s )
{
	std::ostringstream buffer;
	buffer << '"';
	for (std::string::const_iterator it = s.begin () ; it != s.end(); ++it)
	{
		// count backslashes
		unsigned n = 0;
		while (it != s.end () && *it == '\\')
		{
			it++;
			n++;
		}
		if (it == s.end ())
		{
			// at the end of the string we must escape all backslashes,
			// because we are going to append a '"'
			n *= 2;
			for ( unsigned i = 0; i < n; i++ )
			{
				buffer << '\\';
			}
			break;
		}
		else if (*it == '"')
		{
			// with '\'* + '"' we should escape all '\'
			n *= 2;
			// and '"'
			n++;
		}
		for ( unsigned i = 0; i < n; i++ )
		{
			buffer << '\\';
		}
		buffer << *it;
	}
	buffer << '"';
	return buffer.str();
}

static std::string cmd_exe_escape ( const std::string& s )
{
	std::ostringstream buffer;
	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
	{
		if ( (*it == '"')
		     || (*it == ' ')
		     || (*it == '\t')
		     || (*it == '\n')
		     || (*it == '\v')
		     || (*it == '(')
		     || (*it == ')')
		     || (*it == '%')
		     || (*it == '!')
		     || (*it == '^')
		     || (*it == '<')
		     || (*it == '>')
		     || (*it == '&')
		     || (*it == '|')
			)
		{
			buffer << '^';
		}
		buffer << *it;
	}
	return buffer.str();
}

#  define popen_escape(s) cmd_exe_escape(s)
#  define popen_escape2(s) cmd_exe_escape(win_argv_escape(s))
#  define EXEC_TOKEN
#else

static std::string shell_escape(const std::string &s)
{
	std::ostringstream buffer;
	buffer << "'";
	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
	{
		if (*it == '\'')
			buffer << "'\\''";
		else
			buffer << *it;
	}
	buffer << "'";
	return buffer.str();
}

#  define popen_escape(s) shell_escape(s)
#  define popen_escape2(s) shell_escape(s)
#  define EXEC_TOKEN "exec "
#endif

////////////////////////////////////////////////////////////////////////////////

class ZFile {
protected:
    FILE *      _file;
    std::string _filename;
    std::string _mode;

    bool endsWith(const char *str, const char *suffix) {
        size_t strLen = strlen(str);
        size_t suffixLen = strlen(suffix);
        return (suffixLen <= strLen) &&
               (strncmp(&str[strLen - suffixLen], suffix, suffixLen) == 0);
    }

    FILE *processOpen(const std::string &command, const char *mode)
    { return popen(command.c_str(), mode); }

public:
    ZFile(const char *filename, const char *mode="r") {
        if (mode == NULL || (mode[0] != 'r' && mode[0] != 'w'))
            throw std::runtime_error("Invalid mode");

        _filename = filename;
	if(mode[0] == 'r')
	{
            _mode = O_BINARY ? "rb" : "r";
	}
	else if(mode[0] == 'w')
	{
            _mode = O_BINARY ? "wb" : "w";
	}
        ReOpen();
    }
    ~ZFile() { if (_file) fclose(_file); }

    void ReOpen() {
        const char *mode = _mode.c_str();
        if (endsWith(_filename.c_str(), ".gz")) {
            _file = (_mode[0] == 'r') ?
                processOpen(std::string(EXEC_TOKEN "gzip -dc ") + popen_escape2(_filename), mode) :
                processOpen(std::string(EXEC_TOKEN "gzip -c > ") + popen_escape(_filename), mode);
        } else if (endsWith(_filename.c_str(), ".bz2")) {
            _file = (_mode[0] == 'r') ?
                processOpen(std::string(EXEC_TOKEN "bzip2 -dc ") + popen_escape2(_filename), mode) :
                processOpen(std::string(EXEC_TOKEN "bzip2 -c > ") + popen_escape(_filename), mode);
        } else if (endsWith(_filename.c_str(), ".zip")) {
            _file = (_mode[0] == 'r') ?
                processOpen(std::string(EXEC_TOKEN "unzip -c ") + popen_escape2(_filename), mode) :
                processOpen(std::string(EXEC_TOKEN "zip -q > ") + popen_escape(_filename), mode);
        } else { // Assume uncompressed
            _file = fopen(_filename.c_str(), mode);
        }
        if (_file == NULL)
            throw std::runtime_error("Cannot open file");
    }

    operator FILE *() const { return _file; }
};

}

#endif // ZFILE_H
