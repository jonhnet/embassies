/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#if USE_CPPSTDLIB
#include <string>
#endif // USE_CPPSTDLIB
#include "types.h"

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

using namespace std;

class CryptoException
{
public:
	enum ExceptionType { BAD_INPUT, OUT_OF_MEMORY, BUFFER_OVERFLOW, INTEGER_OVERFLOW };

	CryptoException(ExceptionType i_type, const char* i_str, const char* i_filename, const int i_linenum) :
					type(i_type), str(i_str), filename(i_filename), linenum(i_linenum), errorCode(0) { }
	CryptoException(ExceptionType i_type, const char* i_str, uint32_t i_errorCode, const char* i_filename, const int i_linenum) :
					type(i_type), str(i_str), filename(i_filename), linenum(i_linenum), errorCode(i_errorCode) { }
    ~CryptoException() { }

    //void Print ();
#if USE_CPPSTDLIB
	friend ostream& operator<<(ostream& out, CryptoException &e);
#endif // USE_CPPSTDLIB

	ExceptionType getType() { return type; }
	const char *get_str() { return str; }
	const char *get_filename() { return filename; }
	uint32_t get_linenum() { return linenum; }

private:
	ExceptionType type;
    const char* str;
	const char* filename;
	uint32_t linenum;
    uint32_t errorCode;  // Windows error code or Linux errno

	static char* typeToString(ExceptionType i_type);
};

#ifdef NOEXCEPTIONS
#include "LiteLib.h"
#define Throw(...) lite_assert(0)
#else
#define Throw(...) throw CryptoException(__VA_ARGS__, __FILE__, __LINE__)
#endif
