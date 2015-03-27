//#include <stdio.h>	// NULL

#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "xax_util.h"
#include "invoke_debugger.h"
#include "LiteLib.h"

void debug_logfile_append(ZoogDispatchTable_v1 *zdt, const char *logfile_name, const char *message)
{
	debug_logfile_append_f* debug_logfile_append =
		((zdt->zoog_lookup_extension)(DEBUG_LOGFILE_APPEND_NAME));
	(*debug_logfile_append)(logfile_name, message);
}

long DEAD_strace(const char *fn, long val)
{
	return val;
}

uint32_t get_gs_base(void)
{
	int gsval;
	__asm__ __volatile__ (
		"mov	%%gs:(0),%0"
		: "=r" (gsval)
		:
	);
	return gsval;
}

void xax_unimpl_assert(void)
{
	int i=1;
	while (i)
	{
		INVOKE_DEBUGGER_RAW();
		xax_assert(0);
		i+=1;
		// ask compiler to leave an opportunity for
		// a clever fellow with a debugger to escape.
	}
}

void block_forever(ZoogDispatchTable_v1 *zdt)
{
	while (1)
	{
		uint32_t very_dull_zutex = 0;
		ZutexWaitSpec spec;
		spec.zutex = &very_dull_zutex;
		spec.match_val = very_dull_zutex;
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
}
