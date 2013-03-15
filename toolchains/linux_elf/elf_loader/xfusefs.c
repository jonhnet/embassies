#include "xfusefs.h"
#include "xfuse.h"
#include "zqueue.h"

XFuseRequest *xfuserequest_init(MallocFactory *mf, XFuseOperation opn, XfsErr *err)
{
	XFuseRequest *xfr = (XFuseRequest *) mf_malloc(mf, sizeof(XFuseRequest));
	xfr->opn = opn;
	xfr->err = err;
	zevent_init(&xfr->complete_event, false);
	return xfr;
}

void xfuserequest_free(MallocFactory *mf, XFuseRequest *xfr)
{
	zevent_free(&xfr->complete_event);
	mf_free(mf, xfr);
}

#define CREATE_REQUEST(xfs, opn)	\
	XFuseFS *xfusefs = (XFuseFS *) (xfs); \
	XFuseRequest *xfr = xfuserequest_init(xfusefs->mf, xfo_##opn, err); \
	Xfa_##opn *xfa = &xfr->xfa_##opn;

#define ARG(arg) \
	xfa->arg = arg;

#define PROCESS_REQUEST(Type) \
	zq_insert(xfusefs->zqueue, xfr); \
	ZAS *event_zas = &xfr->complete_event.zas; \
	zas_wait_any(xfusefs->xdt, &event_zas, 1);

#define FREE_REQUEST() \
	xfuserequest_free(xfusefs->mf, xfr);

XfsHandle *xfusefs_open(
	XfsErr *err,
	XaxFileSystem *xfs,
	XaxHandleTableIfc *xht,
	XfsPath *path,
	int oflag,
	XaxOpenHooks *xoh)
{
	CREATE_REQUEST(xfs, open);
	ARG(xfs); ARG(xht); ARG(path); ARG(oflag); ARG(xoh);
	PROCESS_REQUEST();

	XfsHandle *result = xfa->result;
	FREE_REQUEST();
	return result;
}

void xfusefs_mkdir(
	XfsErr *err,
	XaxFileSystem *xfs,
	XaxHandleTableIfc *xht,
	XfsPath *path,
	const char *new_name,
	mode_t mode)
{
	CREATE_REQUEST(xfs, mkdir);
	ARG(xfs); ARG(xht); ARG(path); ARG(new_name); ARG(mode);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

void xfusefs_stat64(
	XfsErr *err,
	XaxFileSystem *xfs,
	XfsPath *path,
	struct stat64 *buf)
{
	CREATE_REQUEST(xfs, stat64);
	ARG(xfs); ARG(path); ARG(buf);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

ssize_t xfusefs_read(XfsErr *err, XfsHandle *h, void *buf, size_t count)
{
	CREATE_REQUEST(h->xfs, read);
	ARG(h); ARG(h); ARG(count);
	PROCESS_REQUEST();

	ssize_t result = xfa->result;
	FREE_REQUEST();
	return result;
}

ssize_t xfusefs_write(XfsErr *err, XfsHandle *h, const void *buf, size_t count)
{
	CREATE_REQUEST(h->xfs, write);
	ARG(h); ARG(buf); ARG(count);
	PROCESS_REQUEST();

	ssize_t result = xfa->result;

	FREE_REQUEST();
	return result;
}

void xfusefs_llseek(
	XfsErr *err, XfsHandle *h, int64_t offset, int whence, int64_t *retval)
{
	CREATE_REQUEST(h->xfs, llseek);
	ARG(h); ARG(offset); ARG(whence); ARG(retval);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

void xfusefs_close(XfsErr *err, XfsHandle *h)
{
	CREATE_REQUEST(h->xfs, close);
	ARG(h);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

void xfusefs_fstat64(XfsErr *err, XfsHandle *h, struct stat64 *buf)
{
	CREATE_REQUEST(h->xfs, fstat64);
	ARG(h); ARG(buf);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

void xfusefs_memcpy(
	XfsErr *err, XfsHandle *h, void *dst, size_t len, uint64_t offset)
{
	CREATE_REQUEST(h->xfs, memcpy);
	ARG(h); ARG(dst); ARG(len); ARG(offset);
	PROCESS_REQUEST();
	FREE_REQUEST();
}

int xfusefs_getdent64(XfsErr *err, XfsHandle *h, struct xi_kernel_dirent64 *dirp, uint32_t count)
{
	CREATE_REQUEST(h->xfs, getdent64);
	ARG(h); ARG(dirp); ARG(count);
	PROCESS_REQUEST();

	int result = xfa->result;
	FREE_REQUEST();
	return result;
}

ZAS *xfusefs_get_zas(XfsHandle *h, ZASChannel channel)
{
	XfsErr *err = NULL;	// dummy; common field unused in this fcn.
	CREATE_REQUEST(h->xfs, get_zas);
	ARG(h); ARG(channel);
	PROCESS_REQUEST();
	ZAS *result = xfa->result;
	FREE_REQUEST();
	return result;
}

void xfusefs_init(XFuseFS *xfusefs, MallocFactory *mf, ZoogDispatchTable_v1 *xdt, consume_method_f consume_method)
{
	xfs_define(&xfusefs->xfs,
		xfusefs_open,
		xfusefs_mkdir,
		xfusefs_stat64,
		xfusefs_read,
		xfusefs_write,
		xfusefs_llseek,
		0,
		xfusefs_close,
		xfusefs_fstat64,
		xfusefs_memcpy,
		xfusefs_getdent64,
		xfusefs_get_zas);
	xfusefs->zqueue = zq_init(xdt, mf, consume_method);
	xfusefs->mf = mf;
	xfusefs->xdt = xdt;
}
