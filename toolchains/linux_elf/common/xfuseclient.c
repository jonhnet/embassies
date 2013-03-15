#include <assert.h>

#include "xfuseclient.h"
#include "xfuse.h"
#include "xax_extensions.h"

bool request_consume(void *user_storage, void *user_request)
{
	XFuseRequest **xfr_p = (XFuseRequest **) user_request;
	(*xfr_p) = (XFuseRequest *) user_storage;
	return true;
}

void xfuseclient_init(XFuseClient *xfc, XaxFileSystem *xfs, const char *mount_point)
{
    xfc->child_fs = xfs;
	xe_mount_fuse_client(
		mount_point,
		request_consume,
		&xfc->request_queue,
		&xfc->xdt);
	assert(xfc->request_queue != NULL);
}

void xfuseclient_service_one(XFuseClient *xfc)
{
	XFuseRequest *xfr = NULL;
	zq_consume(xfc->request_queue, &xfr);
	switch (xfr->opn)
	{
	case xfo_open:
	{
		Xfa_open *xfa = &xfr->xfa_open;
		xfa->result = (xfc->child_fs->xfs_open)(
			xfr->err,
			xfc->child_fs,
			xfa->xht,
			xfa->path,
			xfa->oflag,
			xfa->xoh);
		break;
	}
	case xfo_mkdir:
	{
		Xfa_mkdir *xfa = &xfr->xfa_mkdir;
		(xfc->child_fs->xfs_mkdir)(
			xfr->err,
			xfc->child_fs,
			xfa->xht,
			xfa->path,
			xfa->new_name,
			xfa->mode);
		break;
	}
	case xfo_stat64:
	{
		Xfa_stat64 *xfa = &xfr->xfa_stat64;
		(xfc->child_fs->xfs_stat64)(
			xfr->err,
			xfc->child_fs,
			xfa->path,
			xfa->buf);
		break;
	}
	case xfo_read:
	{
		Xfa_read *xfa = &xfr->xfa_read;
		xfa->result = (xfc->child_fs->xfs_read)(
			xfr->err,
			xfa->h,
			xfa->buf,
			xfa->count);
		break;
	}
	case xfo_write:
	{
		Xfa_write *xfa = &xfr->xfa_write;
		xfa->result = (xfc->child_fs->xfs_write)(
			xfr->err,
			xfa->h,
			xfa->buf,
			xfa->count);
		break;
	}
	case xfo_llseek:
	{
		Xfa_llseek *xfa = &xfr->xfa_llseek;
		(xfc->child_fs->xfs_llseek)(
			xfr->err,
			xfa->h,
			xfa->offset,
			xfa->whence,
			xfa->retval);
		break;
	}
	case xfo_close:
	{
		Xfa_close *xfa = &xfr->xfa_close;
		(xfc->child_fs->xfs_close)(
			xfr->err,
			xfa->h);
		break;
	}
    case xfo_fstat64:
    {
        Xfa_fstat64 *xfa = &xfr->xfa_fstat64;
        (xfc->child_fs->xfs_fstat64)(
            xfr->err,
            xfa->h,
            xfa->buf);
        break;
    }
	case xfo_memcpy:
	{
		Xfa_memcpy *xfa = &xfr->xfa_memcpy;
		(xfc->child_fs->xfs_memcpy)(
			xfr->err,
			xfa->h,
			xfa->dst,
			xfa->len,
			xfa->offset);
		break;
	}
	case xfo_getdent64:
	{
		Xfa_getdent64 *xfa = &xfr->xfa_getdent64;
		xfa->result = (xfc->child_fs->xfs_getdent64)(
			xfr->err,
			xfa->h,
			xfa->dirp,
			xfa->count);
		break;
	}
	case xfo_get_zas:
	{
		Xfa_get_zas *xfa = &xfr->xfa_get_zas;
		xfa->result = (xfc->child_fs->xfs_get_zas)(
			xfa->h,
			xfa->channel);
		break;
	}
	default:
		assert(0);	// unhandled case
	}
	zevent_signal(xfc->xdt, &xfr->complete_event, NULL);
}
