#include <base/env.h>
#include <base/printf.h>
#include <base/sleep.h>

void abort(const char *msg)
{
	PDBG("%s", msg);
	Genode::sleep_forever();
}



