/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

// A C++-ification of xax_virtual_ifc. Except we're also
// scrapping file pointers and bounds checks here, putting
// those in the wrapper class that converts to xax_virtual_ifc.

#include <sys/stat.h>	// struct stat64
#include <fcntl.h>	// O_CREAT & friends for open's oflag
#include "XaxVFSTypes.h"
#include "XaxVirtualSocketIfc.h"
#include "zutex_sync.h"	// ZAlwaysReady

typedef enum {
	zas_read=0x80,
	zas_write=0x81,
	zas_except=0x84
} ZASChannel;


class XVFSHandleWrapper;


class XaxVFSHandleIfc {
public:
	virtual ~XaxVFSHandleIfc() {}
	virtual void ftruncate(
		XfsErr *err, int64_t length) = 0;
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf) = 0;

	// stream-type interface -- no file_len info available;
	// implementation determines & returns how many bytes actually avaliable.
	virtual bool is_stream() = 0;
		// return 'false' to get file-type behavior; in that case,
		// implement get_file_len. (I haven't yet figured out how to
		// do this implementation inheritance elegantly in the type
		// system; sorry.)
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len) = 0;
	
	virtual void* fast_mmap(size_t len, uint64_t offset) = 0;
		// returns NULL if no fast-path is available.

	virtual void trace_mmap(size_t len, uint64_t offset, bool fast) = 0;
		// Indicade that an mmap has occured (fast or otherwise);
		// used for recording traces that guide zarfile generation.
		// Purely advisory; no behavior is required.

	// file-type interface -- wrapper maintains a file pointer,
	// computes safe len values automatically.
	virtual uint64_t get_file_len() = 0;	// in bytes
	virtual void read(
		XfsErr *err, void *dst, size_t len, uint64_t offset) = 0;

	// TODO why isn't there a bool is_dir() interface, so we
	// can tell whether getdent or read is the legal call? Fix it.

	// NB there's no stream version of the dir read interface.
	virtual uint32_t get_dir_len(XfsErr *err) = 0;
		// in count of dirents; if you're not a dir, set *err!=XFS_NO_ERROR.
	virtual uint32_t getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count) = 0;
		// returns count of items, NOT buffer size consumed.

	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset) = 0;
	virtual ZAS *get_zas(
		ZASChannel channel) = 0;

	virtual XaxVirtualSocketIfc *as_socket() = 0;
		// returns NULL if this object doesn't support a socket ifc.
};

class XaxVFSHandlePrototype : public XaxVFSHandleIfc  {
public:
	XaxVFSHandlePrototype();

	virtual void ftruncate(
		XfsErr *err, int64_t length);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void read(
		XfsErr *err, void *dst, size_t len, uint64_t offset);
	virtual void* fast_mmap(size_t len, uint64_t offset);
	virtual void trace_mmap(size_t len, uint64_t offset, bool fast);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);
	virtual uint32_t get_dir_len(XfsErr *err);
	virtual uint32_t getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count);
	virtual ZAS *get_zas(
		ZASChannel channel);
	virtual XaxVirtualSocketIfc *as_socket();

protected:
	ZAlwaysReady always_ready;
};

// an ugly callback interface for shoehorning stuff into an XRamFS.
// at least it's optional; most of the time it's unused. Yay!
class XRNode;
class XVOpenHooks {
public:
	virtual ~XVOpenHooks() {}
	virtual XRNode *create_node() = 0;
	virtual mode_t get_mode() = 0;
	virtual void *cheesy_rtti() = 0;
};

class XaxVFSIfc {
public:
	virtual XaxVFSHandleIfc *open(
		XfsErr *err,
		XfsPath *path,
		int oflag,
		XVOpenHooks *open_hooks = NULL) = 0;

	virtual void mkdir(
		XfsErr *err, XfsPath *path, const char *new_name, mode_t mode) = 0;

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf) = 0;

	virtual void hard_link(
		XfsErr *err, XfsPath *src_path, XfsPath *dest_path) = 0;

	virtual void unlink(
		XfsErr *err, XfsPath *path) = 0;

	virtual XaxVirtualSocketProvider *as_socket_provider() = 0;
};

class XaxVFSPrototype : public XaxVFSIfc
{
public:
	virtual void mkdir(
		XfsErr *err, XfsPath *path, const char *new_name, mode_t mode);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

	virtual void hard_link(
		XfsErr *err, XfsPath *src_path, XfsPath *dest_path);

	virtual void unlink(
		XfsErr *err, XfsPath *path);

	virtual XaxVirtualSocketProvider *as_socket_provider();
};

class XaxVFSHandleTableIfc {
public:
	virtual ~XaxVFSHandleTableIfc() {}

	virtual int allocate_filenum() = 0;
	virtual void assign_filenum(int filenum, XaxVFSHandleIfc *hdl) = 0;
	virtual void free_filenum(int filenum) = 0;
	virtual void set_nonblock(int filenum) = 0;
};
