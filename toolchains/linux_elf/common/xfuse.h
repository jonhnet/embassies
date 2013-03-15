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

typedef enum {
	xfo_open,
	xfo_read,
	xfo_write,
	xfo_close,
	xfo_fstat64,
	xfo_stat64,
	xfo_memcpy,
	xfo_get_zas,
	xfo_getdent64,
	xfo_mkdir,
	xfo_llseek,
	xfo_accept,
	xfo_bind,
	xfo_connect,
	xfo_getpeername,
	xfo_getsockname,
	xfo_getsockopt,
	xfo_listen,
	xfo_recvfrom,
	xfo_recv,
	xfo_recvmsg,
	xfo_send,
	xfo_sendmsg,
	xfo_sendto,
	xfo_setsockopt,
	xfo_shutdown,
	xfo_socketset,
} XFuseOperation;

typedef struct {
	XaxFileSystem *xfs;
	XaxHandleTableIfc *xht;
	XfsPath *path;
	int oflag;
	XaxOpenHooks *xoh;
	XfsHandle *result;
} Xfa_open;

typedef struct {
	XaxFileSystem *xfs;
	XaxHandleTableIfc *xht;
	XfsPath *path;
	const char *new_name;
	mode_t mode;
} Xfa_mkdir;

typedef struct {
	XaxFileSystem *xfs;
	XfsPath *path;
	struct stat64 *buf;
} Xfa_stat64;

typedef struct {
	XfsHandle *h;
	void *buf;
	size_t count;
	ssize_t result;
} Xfa_read;

typedef struct {
	XfsHandle *h;
	const void *buf;
	size_t count;
	ssize_t result;
} Xfa_write;

typedef struct {
	XfsHandle *h;
	int64_t offset;
	int whence;
	int64_t *retval;
} Xfa_llseek;

typedef struct {
	XfsHandle *h;
} Xfa_close;

typedef struct {
	XfsHandle *h;
	struct stat64 *buf;
} Xfa_fstat64;

typedef struct {
	XfsHandle *h;
	void *dst;
	size_t len;
	uint64_t offset;
} Xfa_memcpy;

typedef struct {
	XfsHandle *h;
	struct xi_kernel_dirent64 *dirp;
	uint32_t count;
	int result;
} Xfa_getdent64;

typedef struct {
	XfsHandle *h;
	ZASChannel channel;
	ZAS *result;
} Xfa_get_zas;

typedef struct {
	XFuseOperation opn;
	XfsErr *err;
	union {
		Xfa_open xfa_open;
		Xfa_mkdir xfa_mkdir;
		Xfa_stat64 xfa_stat64;
		Xfa_read xfa_read;
		Xfa_write xfa_write;
		Xfa_llseek xfa_llseek;
		Xfa_close xfa_close;
		Xfa_fstat64 xfa_fstat64;
		Xfa_memcpy xfa_memcpy;
		Xfa_getdent64 xfa_getdent64;
		Xfa_get_zas xfa_get_zas;
	};
	ZEvent complete_event;
} XFuseRequest;
