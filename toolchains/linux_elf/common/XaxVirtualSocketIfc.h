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
#include <sys/types.h>
#include <unistd.h>		// socklen_t
#include "zsockaddr.h"
#include "XaxVFSTypes.h"

class XaxVFSIfc;
class XaxVFSHandleIfc;
class XaxVFSHandleTableIfc;

class XaxVirtualSocketProvider
{
public:
	virtual ~XaxVirtualSocketProvider() {}
	virtual void socketset(XaxVFSHandleTableIfc *xht, XaxVFSIfc *xfs, XfsErr *err, int domain, int type, int protocol, XaxVFSHandleIfc **out_handles, int n) = 0;
		// defines socket() and socketpair(). Should split, but I'm
		// ten layers deep in refactoring right now, so drawing the line.
};

class XaxVirtualSocketIfc 
{
public:
	virtual ~XaxVirtualSocketIfc() {}
	virtual int accept(XfsErr *err, XaxVFSHandleTableIfc *xht, ZSOCKADDR_ARG addr, socklen_t *addr_len) = 0;
	virtual int bind(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len) = 0;
	virtual int connect(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len) = 0;
	virtual int getpeername(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len) = 0;
	virtual int getsockname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len) = 0;
	virtual int getsockopt(XfsErr *err, int level, int optname, void *optval, socklen_t *optlen) = 0;
	virtual int listen(XfsErr *err, int n) = 0;
	virtual int recvfrom(XfsErr *err, void *buf, size_t n, int flags, ZSOCKADDR_ARG addr, socklen_t *addr_len) = 0;
	virtual int recv(XfsErr *err, void *buf, size_t n, int flags) = 0;
	virtual int recvmsg(XfsErr *err, struct msghdr *message, int flags) = 0;
	virtual int send(XfsErr *err, __const __ptr_t buf, size_t n, int flags) = 0;
	virtual int sendmsg(XfsErr *err, const struct msghdr *message, int flags) = 0;
	virtual int sendto(XfsErr *err, __const __ptr_t buf, size_t n, int flags, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len) = 0;
	virtual int setsockopt(XfsErr *err, int level, int optname, const __ptr_t optval, socklen_t optlen) = 0;
	virtual int shutdown(XfsErr *err, int how) = 0;
};
