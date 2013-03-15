//#include <stdio.h>	// NULL

#include "pal_abi/pal_abi.h"
#include "xax_util.h"
#include "invoke_debugger.h"
#include "LiteLib.h"

#if 0
// TODO replace with debug_log call.
void leaky_debug_logfile_append(const char *logfile_name, const char *message)
{
	char buf[80];
	strcpy(buf, "debug_");
	strcat(buf, logfile_name);
	int dfd = _xaxunder___open(buf, O_WRONLY|O_CREAT|O_APPEND, 0700);
	_xaxunder___write(dfd, message, strlen(message));
	_xaxunder___close(dfd);
}
#endif

void debug_logfile_append(ZoogDispatchTable_v1 *zdt, const char *logfile_name, const char *message)
{
	typedef void (*debug_logfile_append_t)(const char *logfile_name, const char *message);
	static debug_logfile_append_t debug_logfile_append = NULL;
	if (debug_logfile_append == NULL)
	{
		debug_logfile_append = (debug_logfile_append_t)
			((zdt->zoog_lookup_extension)("debug_logfile_append"));
		if (debug_logfile_append == NULL)
		{
#if 0
			debug_logfile_append = leaky_debug_logfile_append;
#endif
			lite_assert(0);	// no logfile available! Waaaah!
		}
	}

	(*debug_logfile_append)(logfile_name, message);
}

long strace(const char *fn, long val)
{
#if 0
	if (_xax_config->strace)
	{
		char buf[512], hexbuf[16];
		strcpy(buf, fn);
		strcat(buf, "() = ");
		strcat(buf, hexstr(hexbuf, val));
		strcat(buf, "\n");
		debug_logfile_append("strace", buf);
	}
#endif
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
