#include <errno.h>
#include "LiteLib.h"
#include "XVFSHandleWrapper.h"

XVFSHandleWrapper::XVFSHandleWrapper(XaxVFSHandleIfc *hdl)
{
	this->hdl = hdl;
	this->status_flags = O_RDWR;
	this->refcount = 1;
	this->file_pointer = 0;
	this->dir_pointer = 0;
}

XVFSHandleWrapper::~XVFSHandleWrapper()
{
	delete hdl;
}

void XVFSHandleWrapper::add_ref()
{
	refcount+=1;
}

void XVFSHandleWrapper::drop_ref()
{
	refcount-=1;
	if (refcount==0)
	{
		delete this;
	}
}

void XVFSHandleWrapper::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	return hdl->xvfs_fstat64(err, buf);
}

ZAS *XVFSHandleWrapper::get_zas(
	ZASChannel channel)
{
	return hdl->get_zas(channel);
}

XaxVirtualSocketIfc *XVFSHandleWrapper::as_socket()
{
	return hdl->as_socket();
}

uint32_t XVFSHandleWrapper::_bound_count(uint32_t req_count)
{
	uint64_t len = hdl->get_file_len();

	// lite_assert(file_pointer <= len);
	// NB that's not a valid assert: caller could llseek us past eof;
	// we don't know; maybe they're thinking about writing there, getting an
	// implicit file extension. So we want read-past-end to just look EOFy.

	if (file_pointer >= len)
	{
		return 0;
	}
	else if (file_pointer + req_count > len)
	{
		return len - file_pointer;
	}
	else
	{
		return req_count;
	}
}

uint32_t XVFSHandleWrapper::read(
	XfsErr *err, void *dst, size_t len)
{
	ssize_t result;

	if (hdl->is_stream())
	{
		// use stream-type interface
		result = hdl->read(err, dst, len);
	}
	else
	{
		// use file-type interface, managing file-pointer here
		uint32_t actual = _bound_count(len);
		if (actual==0)
		{
			(*err) = XFS_NO_ERROR;
			result = 0;
		}
		else
		{
			hdl->read(err, dst, actual, file_pointer);
			file_pointer += actual;
			result = actual;
		}
	}
	lite_assert((*err) != XFS_DIDNT_SET_ERROR);
	return result;
}

void XVFSHandleWrapper::ftruncate(
	XfsErr *err, int64_t length)
{
	hdl->ftruncate(err, length);
	lite_assert(length >= 0);
	uint64_t ulength = length;

	// maintain sane file pointer invariant
	if (*err == XFS_NO_ERROR)
	{
		lite_assert(hdl->get_file_len() == ulength);
		if (file_pointer > ulength)
		{
			file_pointer = ulength;
		}
	}
}

uint32_t XVFSHandleWrapper::write(
	XfsErr *err, const void *src, size_t len)
{
	uint32_t result;
	if (hdl->is_stream())
	{
		hdl->write(err, src, len, 0);
		result = len;
	}
	else
	{
		uint64_t file_len = hdl->get_file_len();
		if (file_pointer + len > file_len)
		{
			XfsErr terr;
			hdl->ftruncate(&terr, file_pointer+len);
			if (terr != XFS_NO_ERROR)
			{
				// can't grow file
				*err = terr;
				return -1;
			}
		}
		file_len = hdl->get_file_len();
		lite_assert(file_pointer + len <= file_len);
		hdl->write(err, src, len, file_pointer);
		if ((*err) == XFS_NO_ERROR)
		{
			result = len;
			file_pointer += len;
		}
		else
		{
			result = -1;
		}
	}

	lite_assert((*err) != XFS_DIDNT_SET_ERROR);
	return result;
}

uint32_t XVFSHandleWrapper::getdent64(
	XfsErr *err, struct xi_kernel_dirent64 *dirp, uint32_t byte_capacity)
{
	XfsErr dir_len_err;
	uint32_t dir_count = hdl->get_dir_len(&dir_len_err);
	if (dir_len_err!=XFS_NO_ERROR)
	{
		*err = dir_len_err;
		return -1;
	}
	lite_assert(dir_pointer <= dir_count);
	uint32_t dir_remaining = dir_count - dir_pointer;

	// Syscall interface expects us to fill in some integral number
	// of sizeof(xi_kernel_dirent64)'s.
	// XaxVFS ifc assumes that we know
	// how big a struct xi_kernel_dirent64 is, and specify buffer size
	// in those units.
	uint32_t actual = byte_capacity / sizeof(struct xi_kernel_dirent64);
	if (actual > dir_remaining)
	{
		actual = dir_remaining;
	}
	if (actual==0)
	{
		*err = XFS_NO_ERROR;
		return 0;
	}

	uint32_t items_returned = hdl->getdent64(err, dirp, dir_pointer, actual);
	lite_assert(items_returned <= actual);
	if ((*err) == XFS_NO_ERROR)
	{
		dir_pointer += items_returned;
	}
	else
	{
		lite_assert(items_returned == 0);
	}
	return items_returned*sizeof(struct xi_kernel_dirent64);
}

void XVFSHandleWrapper::llseek(
	XfsErr *err, int64_t offset, int whence, int64_t *retval)
{
	if (hdl->is_stream())
	{
		*err = (XfsErr) ESPIPE;
		*retval = -1;
		return;
	}

	uint64_t desired;
	if (whence==SEEK_SET)
	{
		desired = offset;
	}
	else if (whence==SEEK_CUR)
	{
		desired = file_pointer + offset;
	}
	else if (whence==SEEK_END)
	{
		desired = hdl->get_file_len() + offset;
	}
	else
	{
		*err = (XfsErr) EINVAL;
		*retval = -1;
		return;
	}

	// you can seek whereever you want; if it's out of range at read time,
	// you get EOF; if it's out of range at write time, we ftruncate as necessary.

	file_pointer = desired;
	*err = XFS_NO_ERROR;
	*retval = file_pointer;
	return;
}
