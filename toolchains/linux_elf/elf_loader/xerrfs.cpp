#include <errno.h>
#include <alloca.h>

#include "xerrfs.h"
#include "xax_util.h"
#include "xax_posix_emulation.h"

uint32_t XerrHandle::read(
	XfsErr *err, void *dst, size_t len)
{
	// stdin always at EOF
	*err = XFS_NO_ERROR;
	return 0;
}

void XerrHandle::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	char *copy = (char*) alloca(len+1);
	lite_memcpy(copy, src, len);
	copy[len] = '\0';
	debug_logfile_append(xdt, prefix, copy);
	*err = XFS_NO_ERROR;
}

void XerrHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	xnull_fstat64(err, buf, 0);
}

#if 0
void xerr_close(
	XfsErr *err, XfsHandle *xh)
{
	*err = XFS_SILENT_FAIL;
	// meh. I refuse. But I don't want to fail.
}
#endif

XerrHandle::XerrHandle(ZoogDispatchTable_v1 *xdt, const char *prefix)
{
	this->xdt = xdt;
	this->prefix = prefix;
}
