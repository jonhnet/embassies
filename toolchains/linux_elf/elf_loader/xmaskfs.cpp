#include <errno.h>
#include "xmaskfs.h"
#include "xax_posix_emulation.h"

class XMaskFS : public XaxVFSPrototype
{
public:
	virtual XaxVFSHandleIfc *open(
		XfsErr *err,
		XfsPath *path,
		int oflag,
		XVOpenHooks *open_hooks = NULL);
};

XaxVFSHandleIfc *XMaskFS::open(
	XfsErr *err,
	XfsPath *path,
	int oflag,
	XVOpenHooks *open_hooks)
{
	*err = (XfsErr) ENOENT;
	return NULL;
}

void xmask_init(XaxPosixEmulation *xpe)
{
	XMaskFS *xmask = new XMaskFS();

	// Hide these files from zftp; otherwise VNC sees them as zero-length
	// regular files, and freaks out. With them missing, it calmly complains
	// and forges ahead.
	xpe_mount(xpe, mf_strdup(xpe->mf, "/dev/urandom"), xmask);
	xpe_mount(xpe, mf_strdup(xpe->mf, "/dev/random"), xmask);
	xpe_mount(xpe, mf_strdup(xpe->mf, "/etc/ld.so.cache"), xmask);
	xpe_mount(xpe, mf_strdup(xpe->mf, "/etc/ld.so.preload"), xmask);
}
