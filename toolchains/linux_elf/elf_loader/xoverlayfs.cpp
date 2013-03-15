#include <alloca.h>
#include "LiteLib.h"

#include "XVFSHandleWrapper.h"
#include "xoverlayfs.h"

#define ALLOCA_NEWPATH() \
	char *newpath = (char*) alloca(lite_strlen(dest)+lite_strlen(path->suffix)+1); \
	lite_strcpy(newpath, dest); \
	lite_strcat(newpath, path->suffix);

class XOverlayFS : public XaxVFSPrototype
{
public:
	XOverlayFS(XaxPosixEmulation *xpe, const char *dest);

	virtual XaxVFSHandleIfc *open(
		XfsErr *err,
		XfsPath *path,
		int oflag,
		XVOpenHooks *open_hooks = NULL);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

private:
	XaxPosixEmulation *xpe;
	const char *dest;
};

XOverlayFS::XOverlayFS(XaxPosixEmulation *xpe, const char *dest)
{
	this->xpe = xpe;
	this->dest = dest;
}

XaxVFSHandleIfc *XOverlayFS::open(
	XfsErr *err,
	XfsPath *path,
	int oflag,
	XVOpenHooks *open_hooks)
{
	ALLOCA_NEWPATH();
	XVFSHandleWrapper *wrapper =
		xpe_open_hook_handle(xpe, err, newpath, oflag, open_hooks);
	return wrapper->get_underlying_handle();
}

void XOverlayFS::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	ALLOCA_NEWPATH();
	xpe_stat64(xpe, err, newpath, buf);
}

void xoverlayfs_init(XaxPosixEmulation *xpe, const char *dest, const char *srcpath)
{
	xpe_mount(xpe, srcpath, new XOverlayFS(xpe, dest));
}
