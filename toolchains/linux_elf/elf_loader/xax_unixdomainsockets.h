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

#include "xax_posix_emulation.h"

void xax_unixdomainsockets_init(XaxPosixEmulation *xpe);

//////////////////////////////////////////////////////////////////////////////
// Below this line, these classes are all internal to the implementation
// of unix-domain sockets. They're here just to organize the implementation
// into class defs (.h) and method imples (.cpp).

#include "XRamFS.h"
#include "ZQueue.h"

class XUDSMessage {
public:
	XUDSMessage(MallocFactory *mf, const void *content, uint32_t count);
	~XUDSMessage();

	uint32_t count;
	uint8_t *content;
	uint32_t offset;
	MallocFactory *mf;
};


class XUDSFileSystem : public XaxVFSPrototype, public XaxVirtualSocketProvider
{
public:
	XUDSFileSystem(XaxPosixEmulation *xpe);

	MallocFactory *get_mf() { return mf; }
	ZoogDispatchTable_v1 *get_xdt() { return xpe->zdt; }

	XaxPosixEmulation *get_xpe() { return xpe; }

	XaxVirtualSocketProvider *as_socket_provider() { return this; }

	virtual XaxVFSHandleIfc *open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks = NULL);

	void socketset(XaxVFSHandleTableIfc *xht, XaxVFSIfc *xfs, XfsErr *err, int domain, int type, int protocol, XaxVFSHandleIfc **out_handles, int n);

	void fill_ucred(struct ucred *cr);
	
private:
	MallocFactory *mf;
	XaxPosixEmulation *xpe;
};

typedef enum {
	xt_accept_end = 0,
	xt_connect_end = 1,
	xt_bind,
	xt_unset,
	xt_broken,
} XUDSEndpointType;

class XUDSEndpoint
{
public:
	XUDSEndpoint(XUDSFileSystem *xufs, consume_method_f *consume_method, const char *address);
	~XUDSEndpoint();

	XUDSFileSystem *get_xufs() { return xufs; }
	char *get_address() { return address; }

	ZAS *get_zas();

protected:
	XUDSFileSystem *xufs;
	ZQueue *zq;		/* ZQueue<(XUDSMessage|XUDSConnection)*> */
	char *address;
};

class XUDSMessageEndpoint : public XUDSEndpoint
{
public:
	XUDSMessageEndpoint(XUDSFileSystem *xufs, const char *address);
	~XUDSMessageEndpoint();

	void enqueue(XUDSMessage *xum);

	uint32_t consume_part_of_message(void *buf, uint32_t count);
	ZAS *get_zas();

private:
	class MessageConsumer
	{
	public:
		MessageConsumer(void *buf, uint32_t count, bool discard_rest_of_message);
		static bool consume_func(void *user_storage, void *user_request);

	public:
		void *buf;
		uint32_t count;
		bool discard_rest_of_message;
			// not yet used;
			// for if we ever want SOCK_DGRAMs as well as SOCK_STREAMs.
		uint32_t out_actual;
	};
};

class XUDSConnectionEndpoint;

class XUDSConnection
{
public:
	XUDSConnection(XUDSConnectionEndpoint *bind_ep);

	XUDSMessageEndpoint *ep[2];
		// handle uh reads from uh->connection->ep[uh->bind_type]
		// handle uh writes into uh->connection->ep[1-uh->bind_type]
	XUDSFileSystem *xufs;

	void add_ref();
	void drop_ref(XUDSEndpointType ept);

private:
	int refcount;				// TODO synch lock
	bool refs[2];

	~XUDSConnection();
};


class XUDSConnectionEndpoint : public XUDSEndpoint
{
public:
	XUDSConnectionEndpoint(XUDSFileSystem *xufs, const char *address);
	~XUDSConnectionEndpoint();

	void enqueue(XUDSConnection *connection);
	XUDSConnection *consume();

private:
	class ConnectionConsumer
	{
	public:
		static bool consume_func(void *user_storage, void *v_this);
	public:
		XUDSConnection *out_consumed_conn;
	};
};

class XUDSHandle : public XRNodeHandle, public XaxVirtualSocketIfc
{
public:
	XUDSHandle(XUDSFileSystem *xufs);
	~XUDSHandle();

	XUDSEndpointType get_bind_type() { return bind_type; };

//	virtual void ftruncate(
//		XfsErr *err, int64_t length);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);
//	virtual uint32_t getdent64(
//		XfsErr *err, struct xi_kernel_dirent64 *dirp,
//		uint32_t item_offset, uint32_t item_count);
		// returns count of items, NOT buffer size consumed.
	virtual ZAS *get_zas(
		ZASChannel channel);

	virtual bool is_stream() { return true; }
	uint64_t get_file_len() { lite_assert(false); return 0; }

	virtual XaxVirtualSocketIfc *as_socket() { return this; }

	int accept(XfsErr *err, XaxVFSHandleTableIfc *xht, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	int bind(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	int connect(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	int getpeername(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	int getsockname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	int getsockopt(XfsErr *err, int level, int optname, void *optval, socklen_t *optlen);
	int listen(XfsErr *err, int n);
	int recvfrom(XfsErr *err, void *buf, size_t n, int flags, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	int recv(XfsErr *err, void *buf, size_t n, int flags);
	int recvmsg(XfsErr *err, struct msghdr *message, int flags);
	int send(XfsErr *err, __const __ptr_t buf, size_t n, int flags);
	int sendmsg(XfsErr *err, const struct msghdr *message, int flags);
	int sendto(XfsErr *err, __const __ptr_t buf, size_t n, int flags, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	int setsockopt(XfsErr *err, int level, int optname, const __ptr_t optval, socklen_t optlen);
	int shutdown(XfsErr *err, int how);

	XUDSFileSystem *get_xufs() { return xufs; }
	void become_bind(const char *address);
	void become_connect(XUDSHandle *bind_handle);
	void become_accept(XUDSConnection *conn);

	XUDSHandle *accept_internal();

private:
	XUDSFileSystem *xufs;
	XUDSEndpointType bind_type;
	XUDSConnectionEndpoint *bind_ep;		/* ZQueue<XUDSConnection*> */
	XUDSConnection *connection;

	void _insert_message(XUDSMessage *xum);
	int _getepname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len, XUDSEndpointType whichep);
};

class XRBindNode : public XRNode
{
public:
	XRBindNode(XUDSHandle *bind_handle);

	XRNodeHandle *open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh);
	void mkdir_node(XfsErr *err, const char *new_name, mode_t mode);

	XUDSFileSystem *get_xufs() { return bind_handle->get_xufs(); }

private:
	XUDSHandle *bind_handle;
};

