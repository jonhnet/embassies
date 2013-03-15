#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <alloca.h>

#include "LiteLib.h"
#include "XRamFS.h"
#include "XRFileNode.h"
#include "XRDirNode.h"
#include "malloc_factory.h"
#include "DecomposePath.h"

XRNode::XRNode(XRamFS *xramfs)
{
	this->xramfs = xramfs;
	this->refcount = 1;
}

void XRNode::add_ref()
{
	this->refcount += 1;
}

void XRNode::drop_ref()
{
	// TODO lock
	this->refcount -= 1;
	if (this->refcount==0)
	{
		delete this;
	}
}

XRNode *XRNode::hard_link()
{
	return NULL;
}

void XRNode::unlink_node(XfsErr *err, char *child)
{
	*err = (XfsErr) EINVAL;
}

XRNodeHandle::XRNodeHandle(XRNode *xrnode)
{
	this->xrnode = xrnode;
}

//////////////////////////////////////////////////////////////////////////////

XRamFS::XRamFS(MallocFactory *mf, FakeIds *fake_ids)
{
	this->mf = mf;
	// order matters here; XRDirNode's ctor calls back into me to get my mf.
	this->fake_ids = fake_ids;
	this->root = new XRDirNode(this);
}

XaxVFSHandleIfc *XRamFS::open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *xoh)
{
	return root->open_node(err, path->suffix, oflag, xoh);
}

class RDONLYOpenHook : public XVOpenHooks
{
public:
	virtual XRNode *create_node() { return NULL; }
	virtual mode_t get_mode() { return O_RDONLY; }
	virtual void *cheesy_rtti() { static int rtti; return &rtti; }
};

void XRamFS::mkdir(
	XfsErr *err, XfsPath *path, const char *new_name, mode_t mode)
{
	// Open the dir...
	RDONLYOpenHook xoh;
	XRNodeHandle *xnh = root->open_node(err, path->suffix, O_RDWR, &xoh);
	if (*err!=XFS_NO_ERROR)
	{
		return;
	}

	// make the new subdir...
	xnh->get_xrnode()->mkdir_node(err, new_name, mode);

	// close the dir handle
	delete xnh;
}

void XRamFS::stat64(XfsErr *err, XfsPath *path, struct stat64 *buf)
{
	*err = XFS_ERR_USE_FSTAT64;
}

class HardLinkOpenHook : public XVOpenHooks
{
public:
	HardLinkOpenHook(XRNode *target) : target(target) {}
	virtual XRNode *create_node() { return target; }
	virtual mode_t get_mode() { return O_RDWR; }
	virtual void *cheesy_rtti() { static int rtti; return &rtti; }
private:
	XRNode *target;
};

void XRamFS::hard_link(XfsErr *err, XfsPath *src_path, XfsPath *dest_path /*, const char *new_name*/)
{
	XRNodeHandle *src_handle = NULL;
	XRNodeHandle *dest_handle = NULL;
	RDONLYOpenHook xoh;
	src_handle = root->open_node(err, src_path->suffix, O_RDWR, &xoh);
	if (src_handle==NULL)
	{
		*err = (XfsErr) ENOENT;
		goto fail;
	}
	XRNode *link_target;
	link_target = src_handle->get_xrnode()->hard_link();
	if (link_target==NULL)
	{
		*err = (XfsErr) EINVAL;
		goto fail;
	}

	{
		HardLinkOpenHook hlhook(link_target);
		dest_handle = root->open_node(err, dest_path->suffix, O_CREAT|O_RDWR, &hlhook);
	}
	lite_assert(dest_handle!=NULL);

	*err = XFS_NO_ERROR;
fail:
	delete dest_handle;
	delete src_handle;
}

void XRamFS::unlink(XfsErr *err, XfsPath *path)
{
	DecomposePath dpath(mf, path->suffix);
	if (dpath.err!=XFS_NO_ERROR)
	{
		*err = (XfsErr) dpath.err;
		return;
	}

	RDONLYOpenHook xoh;
	XRNodeHandle *hdl = root->open_node(err, dpath.prefix, O_RDWR, &xoh);
	if (hdl==NULL)
	{
		*err = (XfsErr) ENOENT;
		return;
	}
	XRNode *xrn;
	xrn = hdl->get_xrnode();
	xrn->unlink_node(err, dpath.tail);
}

FakeIds *XRamFS::get_fake_ids()
{
	return fake_ids;
}

void XRamFS::set_fake_ids(FakeIds *fake_ids)
{
	this->fake_ids = fake_ids;
}

XRamFS *xax_create_ramfs(XaxPosixEmulation *xpe, const char *path)
{
	MallocFactory *mf = xpe->mf;
	XRamFS *ramfs = new XRamFS(mf, &xpe->fake_ids);

	xpe_mount(xpe, mf_strdup(mf, path), ramfs);
	return ramfs;
}

