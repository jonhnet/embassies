#include "pal_abi/pal_types.h"
#include "XStreamXdt.h"

void _xsxdt_emit(XStream *xs, const char *s)
{
	XStreamXdt *xsx = (XStreamXdt *) xs;
	if (xsx->dla!=NULL)
	{
		(xsx->dla)("stderr", s);
	}
}

void xsxdt_init(XStreamXdt *xsx, ZoogDispatchTable_v1 *xdt, const char *dbg_stream_name)
{
	xsx->xstream.emit = _xsxdt_emit;
	xsx->dbg_stream_name = dbg_stream_name;
	xsx->dla = (debug_logfile_append_f *)
		(xdt->zoog_lookup_extension)("debug_logfile_append");
}
