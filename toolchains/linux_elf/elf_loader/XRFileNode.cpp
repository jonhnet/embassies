#include <errno.h>

#include "LiteLib.h"
#include "XRFileNode.h"

XRFileNode::XRFileNode(XRamFS *xrfs)
	: XRNode(xrfs)
{
	_init(xrfs->get_mf());
}

XRFileNode::XRFileNode(MallocFactory *mf)
	: XRNode(NULL)
{
	_init(mf);
}

XRNodeHandle *XRFileNode::open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh)
{
	lite_assert(path[0]=='\0');
	XRFileNodeHandle *hdl = new XRFileNodeHandle(this);

	*err = XFS_NO_ERROR;
	return hdl;
}

void XRFileNode::mkdir_node(XfsErr *err, const char *new_name, mode_t mode)
{
	lite_assert(0);
}

XRNode *XRFileNode::hard_link()
{
	add_ref();
	return this;
}

void XRFileNode::_init(MallocFactory *mf)
{
	this->mf = mf;
	this->capacity = 1024;
	this->contents = (uint8_t*) mf_malloc(mf, this->capacity);
	this->size = 0;
}

void XRFileNode::_expand(uint32_t min_new_capacity)
{
	uint32_t new_capacity = capacity;
	while (new_capacity < min_new_capacity)
	{
		new_capacity = new_capacity << 1;
	}
	if (new_capacity != capacity)
	{
		contents = (uint8_t*) mf_realloc(mf, contents, capacity, new_capacity);
		lite_memset(&contents[size], 0, new_capacity - capacity);
		capacity = new_capacity;
	}
	lite_assert(min_new_capacity <= capacity);
}

XRFileNodeHandle::XRFileNodeHandle(XRFileNode *xrfnode)
	: XRNodeHandle(xrfnode)
{
}

uint64_t XRFileNodeHandle::get_file_len()
{
	return get_file_node()->size;
}

void XRFileNodeHandle::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset)
{
	uint32_t avail = get_file_node()->size - offset;
	if (avail<0) { avail = 0; }
	lite_assert(len <= avail);	// supposed to be handled a layer up

	if (len > 0)
	{
		lite_memcpy(dst, &get_file_node()->contents[offset], len);
	}
	*err = XFS_NO_ERROR;
}

void XRFileNodeHandle::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	get_file_node()->_expand(offset+len);

	lite_memcpy(&get_file_node()->contents[offset], src, len);

	if (offset+len > get_file_node()->size)
	{
		get_file_node()->size = offset+len;
	}
	*err = XFS_NO_ERROR;
}

void XRFileNodeHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	xnull_fstat64(err, buf, get_file_node()->size);
}

void XRFileNodeHandle::ftruncate(
	XfsErr *err, int64_t length)
{
	if (length < 0)
	{
		*err = (XfsErr) EINVAL;
		return;
	}
	
	lite_assert(length < (uint32_t) 0xfffffffe);	// next call not 64-bit ready

	get_file_node()->_expand(length);
	get_file_node()->size = length;

	*err = XFS_NO_ERROR;
}

