#include "exception_test.h"

class BlueException {
};

void foo()
{
#ifdef __EXCEPTIONS
	throw BlueException();
#endif
}

const char *exception_test()
{
#ifdef __EXCEPTIONS
	try {
		foo();
	} catch (BlueException ex) {
		return "success";
	}
	return "failure";
#else
	return "disabled";
#endif
}

