#include <stdio.h>
#include "LiteLib.h"
#include "safety_check.h"

void safety_violation(const char *file, int line, const char *failed_predicate)
{
	fprintf(stderr, "safety_violation at %s:%d: !%s. Failing zoog call.\n",
		file, line, failed_predicate);
#if DEBUG_VULNERABLY
	lite_assert(false);
#endif // DEBUG_VULNERABLY
}
