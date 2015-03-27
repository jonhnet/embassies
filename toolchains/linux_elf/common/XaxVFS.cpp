#include <errno.h>	// ENOSYS
#include "LiteLib.h"
#include "XaxVFS.h"
#include "xax_util.h"

XaxVFSHandlePrototype::XaxVFSHandlePrototype()
{
	zar_init(&always_ready);
}

void XaxVFSHandlePrototype::ftruncate(
	XfsErr *err, int64_t length)
{
	lite_assert(0 && "unimpl");
}

void XaxVFSHandlePrototype::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	lite_assert(0 && "unimpl");
}

uint32_t XaxVFSHandlePrototype::read(
	XfsErr *err, void *dst, size_t len)
{
	lite_assert(0 && "unimpl");
	return -1;
}

void XaxVFSHandlePrototype::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset)
{
	lite_assert(0 && "unimpl");
}

void* XaxVFSHandlePrototype::fast_mmap(
	size_t len, uint64_t offset)
{
	return NULL;
}

void XaxVFSHandlePrototype::trace_mmap(size_t len, uint64_t offset, bool fast)
{
}

void XaxVFSHandlePrototype::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	lite_assert(0 && "unimpl");
}

uint32_t XaxVFSHandlePrototype::get_dir_len(XfsErr *err)
{
	lite_assert(0 && "unimpl; probably not a directory");
	return -1;
}

uint32_t XaxVFSHandlePrototype::getdent64(
	XfsErr *err, struct xi_kernel_dirent64 *dirp,
	uint32_t item_offset, uint32_t item_count)
{
	lite_assert(0 && "unimpl");
	return -1;
}

ZAS *XaxVFSHandlePrototype::get_zas(
	ZASChannel channel)
{
	switch (channel)
	{
		case zas_read:
			return &always_ready.zas;
		case zas_write:
			return &always_ready.zas;
		case zas_except:
			xax_assert(0);	// unimpl; could be a real zevent that's never signaled
			//return &never_ready;
		default:
			xax_assert(0);	// bogus param
	}
}

XaxVirtualSocketIfc *XaxVFSHandlePrototype::as_socket()
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void XaxVFSPrototype::mkdir(
	XfsErr *err, XfsPath *path, const char *new_name, mode_t mode)
{
	lite_assert(0 && "unimpl");
}

void XaxVFSPrototype::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	*err = XFS_ERR_USE_FSTAT64;
	// Ask XPE to use open/fstat/close instead.
}

void XaxVFSPrototype::hard_link(
	XfsErr *err, XfsPath *src_path, XfsPath *dest_path/*, const char *new_name*/)
{
	lite_assert(0 && "unimpl");
}

void XaxVFSPrototype::unlink(
	XfsErr *err, XfsPath *path)
{
	*err = (XfsErr) -ENOSYS;
}

XaxVirtualSocketProvider *XaxVFSPrototype::as_socket_provider()
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////
