#include <alloca.h>
#include "LiteLib.h"

#include "xrandomfs.h"

class XRandomHandle : public XaxVFSHandlePrototype
{
public:
	virtual bool is_stream() { return true; }
	virtual uint64_t get_file_len() { lite_assert(false); return 0; }
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);

};

uint32_t XRandomHandle::read(
	XfsErr *err, void *dst, size_t len)
{
	// Implementation reference:
	// http://dilbert.com/strips/comic/2001-10-25/
	lite_memset(dst, 9, len);
	*err = XFS_NO_ERROR;
	return len;
}

void XRandomHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	buf->st_dev = 5;
	buf->st_ino = 555;
	buf->st_mode = 0x21b6;
	buf->st_nlink = 1;
	buf->st_uid = 0;
	buf->st_gid = 0;
	buf->st_size = 0;
	buf->st_blksize = 512;
	buf->st_blocks = 0;
	buf->st_atime = 1299393405;
	buf->st_mtime = 1299393405;
	buf->st_ctime = 1299393405;
	*err = XFS_NO_ERROR;
	return;
}

class XRandomFS : public XaxVFSPrototype
{
public:
	virtual XaxVFSHandleIfc *open(
		XfsErr *err,
		XfsPath *path,
		int oflag,
		XVOpenHooks *open_hooks = NULL);
};

XaxVFSHandleIfc *XRandomFS::open(
	XfsErr *err,
	XfsPath *path,
	int oflag,
	XVOpenHooks *open_hooks)
{
	*err = XFS_NO_ERROR;
	return new XRandomHandle();
}

void xrandomfs_init(XaxPosixEmulation *xpe, const char *location)
{
// TODO could probably skip the XRandomFS part and plug this into an XRamFS?
	XRandomFS *xrandomfs = new XRandomFS();
	xpe_mount(xpe, location, xrandomfs);
}
