#include "FilterVFS.h"
#include "LiteLib.h"

FilterVFSHandle::FilterVFSHandle(XaxVFSHandleIfc *base_handle)
{
	this->base_handle = base_handle;
}

FilterVFSHandle::~FilterVFSHandle()
{
	delete base_handle;
}

void FilterVFSHandle::ftruncate(
	XfsErr *err, int64_t length)
{
	base_handle->ftruncate(err, length);
}

void FilterVFSHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	base_handle->xvfs_fstat64(err, buf);
}

uint32_t FilterVFSHandle::read(
	XfsErr *err, void *dst, size_t len)
{
	lite_assert(false);
	return -1;
}

void FilterVFSHandle::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset)
{
	base_handle->read(err, dst, len, offset);
}

void FilterVFSHandle::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	base_handle->write(err, src, len, offset);
}

void* FilterVFSHandle::fast_mmap(size_t len, uint64_t offset)
{
	return NULL;
}

uint32_t FilterVFSHandle::get_dir_len(XfsErr *err)
{
	return base_handle->get_dir_len(err);
}

uint32_t FilterVFSHandle::getdent64(
	XfsErr *err, struct xi_kernel_dirent64 *dirp,
	uint32_t item_offset, uint32_t item_count)
{
	return base_handle->getdent64(err, dirp, item_offset, item_count);
}

ZAS *FilterVFSHandle::get_zas(
	ZASChannel channel)
{
	return base_handle->get_zas(channel);
}

uint64_t FilterVFSHandle::get_file_len()
{
	return base_handle->get_file_len();
}

//////////////////////////////////////////////////////////////////////////////

FilterVFS::FilterVFS(XaxVFSIfc *base_vfs)
{
	this->base_vfs = base_vfs;
}

FilterVFS::~FilterVFS()
{
	delete base_vfs;
}

void FilterVFS::mkdir(
	XfsErr *err, XfsPath *path, const char *new_name, mode_t mode)
{
	base_vfs->mkdir(err, path, new_name, mode);
}

void FilterVFS::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	base_vfs->stat64(err, path, buf);
}
