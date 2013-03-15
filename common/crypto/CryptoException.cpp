#if USE_CPPSTDLIB
#include <iostream>
#include <string.h>
#endif // USE_CPPSTDLIB
#include "CryptoException.h"

using namespace std;

#if USE_CPPSTDLIB
ostream& operator<<(ostream& out, CryptoException &e) {
	out << "Exception: " << e.filename << ":" << e.linenum << " " 
		<< CryptoException::typeToString(e.type) << " - " << e.str << endl;

#ifdef _WIN32
    if (e.errorCode != 0) {
        LPSTR lpMsgBuf = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       GetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR) &lpMsgBuf,
                       0,
                       NULL);
		out << "        error code: " << e.errorCode << " / 0x" 
			<< hex << e.errorCode << " : " << lpMsgBuf << endl;
 
        LocalFree(lpMsgBuf);
    }
#else
    if (e.errorCode != 0) {
      out << "      error code: " << e.errorCode << ": " << strerror(e.errorCode) << endl;
    }
#endif // _WIN32

	return out;
}
#endif // USE_CPPSTDLIB


char* CryptoException::typeToString(ExceptionType i_type) {
	switch(i_type) {
		case CryptoException::BAD_INPUT:
			return (char*)"Bad input";
		case CryptoException::OUT_OF_MEMORY:
			return (char*)"Ran out of memory";
		case CryptoException::BUFFER_OVERFLOW:
			return (char*)"Exceeded the bounds of a buffer";
		case CryptoException::INTEGER_OVERFLOW:
			return (char*)"Arithmetic operation overflowed";
		default:
			return (char*)"Unknown";
	}
}
