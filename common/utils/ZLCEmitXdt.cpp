#include <stdio.h>
#include "pal_abi/pal_abi.h"
#include "ZLCEmitXdt.h"

ZLCEmitXdt::ZLCEmitXdt(ZoogDispatchTable_v1 *zdt, ZLCChattiness threshhold)
{
	this->_threshhold = threshhold;
	debuglog = (debug_logfile_append_f*)
		(zdt->zoog_lookup_extension)("debug_logfile_append");
}

void ZLCEmitXdt::emit(const char *s)
{
	if (debuglog != NULL)
	{
		(debuglog)("stderr", s);
	}
}
