#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/socket.h>	// SO_REUSEADDR
#include <errno.h>

#include "LiteLib.h"
#include "xax_unixdomainsockets.h"
#include "ZQueue.h"
#include "XRamFS.h"
#include "malloc_factory.h"
#include "debug_util.h"
#include "cheesy_snprintf.h"
#include "XVFSHandleWrapper.h"
#include "xax_util.h"
#include "math_util.h"

XUDSMessage::XUDSMessage(MallocFactory *mf, const void *content, uint32_t count)
{
	lite_assert(count>0);
	this->content = (uint8_t*) mf_malloc(mf, count);
	this->count = count;
	this->offset = 0;
	this->mf = mf;
	memcpy(this->content, content, count);
}

XUDSMessage::~XUDSMessage()
{
	mf_free(mf, content);
}

//////////////////////////////////////////////////////////////////////////////

XUDSEndpoint::XUDSEndpoint(XUDSFileSystem *xufs, consume_method_f *consume_method, const char *address)
{
	this->xufs = xufs;
	this->zq = zq_init(xufs->get_xdt(), xufs->get_mf(), consume_method);
	this->address = mf_strdup(xufs->get_mf(), address);
}

XUDSEndpoint::~XUDSEndpoint()
{
	mf_free(xufs->get_mf(), address);
	lite_assert(zq_is_empty(zq));	// it's superclass's job to drain the queue
	zq_free(zq);
}

ZAS *XUDSEndpoint::get_zas()
{
	return zq_get_zas(zq);
}

//////////////////////////////////////////////////////////////////////////////

XUDSConnectionEndpoint::XUDSConnectionEndpoint(XUDSFileSystem *xufs, const char *address)
	: XUDSEndpoint(xufs, &XUDSConnectionEndpoint::ConnectionConsumer::consume_func, address)
{
}

XUDSConnectionEndpoint::~XUDSConnectionEndpoint()
{
	// sorry, don't know how to drain unwanted connections yet!
	// I guess we'd have to wake them and return some error? ugh!
}

void XUDSConnectionEndpoint::enqueue(XUDSConnection *connection)
{
	connection->add_ref();
	zq_insert(zq, connection);
}

XUDSConnection *XUDSConnectionEndpoint::consume()
{
	XUDSConnectionEndpoint::ConnectionConsumer cc;
	zq_consume(zq, &cc);
	return cc.out_consumed_conn;
}

bool XUDSConnectionEndpoint::ConnectionConsumer::consume_func(void *user_storage, void *v_this)
{
	XUDSConnection *conn = (XUDSConnection *) user_storage;
	ConnectionConsumer *c_this = (ConnectionConsumer *) v_this;
	c_this->out_consumed_conn = conn;
	return true;
}

//////////////////////////////////////////////////////////////////////////////

XUDSMessageEndpoint::XUDSMessageEndpoint(XUDSFileSystem *xufs, const char *address)
	: XUDSEndpoint(xufs, &XUDSMessageEndpoint::MessageConsumer::consume_func, address)
{
}

XUDSMessageEndpoint::~XUDSMessageEndpoint()
{
	while (!zq_is_empty(zq))
	{
		// drop unread messages. Should probably warn loudly when this happens.
		XUDSMessageEndpoint::MessageConsumer mc(NULL, 0, true);
		zq_consume(zq, &mc);
	}
}

void XUDSMessageEndpoint::enqueue(XUDSMessage *message)
{
	zq_insert(zq, message);
}

uint32_t XUDSMessageEndpoint::consume_part_of_message(void *buf, uint32_t count)
{
	XUDSMessageEndpoint::MessageConsumer mc(buf, count, false);
	zq_consume(zq, &mc);
	lite_assert(mc.out_actual!=(uint32_t)-1);
	return mc.out_actual;
}

XUDSMessageEndpoint::MessageConsumer::MessageConsumer(void *buf, uint32_t count, bool discard_rest_of_message)
{
	this->buf = buf;
	this->count = count;
	this->discard_rest_of_message = discard_rest_of_message;
	this->out_actual = (uint32_t)-1;
}

bool XUDSMessageEndpoint::MessageConsumer::consume_func(void *user_storage, void *v_this)
{
	XUDSMessage *xum = (XUDSMessage *) user_storage;
	MessageConsumer *mcr = (MessageConsumer *) v_this;

	uint32_t avail = xum->count - xum->offset;
	uint32_t actual = mcr->count;
	if (avail<mcr->count)
	{
		actual = avail;
	}
	xax_assert(actual<=mcr->count);

	if (mcr->buf==NULL)
	{
		lite_assert(actual==0);
	}
	else
	{
		memcpy(mcr->buf, xum->content+xum->offset, actual);
	}
	mcr->out_actual = actual;

	xum->offset += actual;
	xax_assert(xum->offset <= xum->count);
	bool consumed = mcr->discard_rest_of_message || (xum->offset == xum->count);
	if (consumed)
	{
		delete xum;
	}
	return consumed;
}

//////////////////////////////////////////////////////////////////////////////

XUDSConnection::XUDSConnection(XUDSConnectionEndpoint *bind_ep)
{
	this->xufs = bind_ep->get_xufs();
	this->ep[xt_accept_end] = new XUDSMessageEndpoint(xufs, bind_ep->get_address());
	this->ep[xt_connect_end] = new XUDSMessageEndpoint(xufs, "");
	this->refcount = 1;
	this->refs[xt_accept_end] = false;
	this->refs[xt_connect_end] = true;
}

XUDSConnection::~XUDSConnection()
{
	lite_assert(this->refs[xt_accept_end] == false);
	lite_assert(this->refs[xt_connect_end] == false);
	delete ep[xt_accept_end];
	delete ep[xt_connect_end];
}

void XUDSConnection::add_ref()
{
	refcount+=1;
	lite_assert(this->refs[xt_accept_end] == false);
	this->refs[xt_accept_end] = true;
}

void XUDSConnection::drop_ref(XUDSEndpointType ept)
{
	lite_assert(ept==xt_accept_end || ept==xt_connect_end);
	lite_assert(this->refs[ept] == true);
	this->refs[ept] = false;

	lite_assert(refcount>=1);
	// TODO lock
	refcount-=1;
	if (refcount==0)
	{
		delete this;
	}
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

XUDSHandle::XUDSHandle(XUDSFileSystem *xufs)
	: XRNodeHandle(NULL)
{
	this->xufs = xufs;
	this->bind_type = xt_unset;
	this->bind_ep = NULL;
	this->connection = NULL;
}

//////////////////////////////////////////////////////////////////////////////

class BindHook; 
class ConnectHook; 

class XUDSHook : public XVOpenHooks
{
public:
	virtual BindHook *as_bind_hook() = 0;
	virtual ConnectHook *as_connect_hook() = 0;
	virtual void *cheesy_rtti() { static int rtti; return &rtti; }
};

class BindHook : public XUDSHook
{
public:
	BindHook(XUDSHandle *uh);
	virtual XRNode *create_node();
	virtual mode_t get_mode() { return 0777; }
	virtual BindHook *as_bind_hook() { return this; }
	virtual ConnectHook *as_connect_hook() { return NULL; }
private:
	XRBindNode *bind_node;
};

BindHook::BindHook(XUDSHandle *uh)
{
	xax_assert(uh->get_bind_type() == xt_bind);
	this->bind_node = new XRBindNode(uh);
}

XRNode *BindHook::create_node()
{
	return bind_node;
}

class ConnectHook : public XUDSHook
{
public:
	ConnectHook(XUDSHandle *uh);
	XUDSHandle *create_connect_handle();

	virtual XRNode *create_node() { lite_assert(false); return NULL; }
	virtual mode_t get_mode() { return 0777; }
	virtual BindHook *as_bind_hook() { return NULL; }
	virtual ConnectHook *as_connect_hook() { return this; }

private:
	XUDSHandle *uh;
};

ConnectHook::ConnectHook(XUDSHandle *uh)
{
	this->uh = uh;
}

XUDSHandle *ConnectHook::create_connect_handle()
{
	return uh;
}

//////////////////////////////////////////////////////////////////////////////

XRBindNode::XRBindNode(XUDSHandle *bind_handle)
	: XRNode(NULL) 	// where would I get this value?
{
	this->bind_handle = bind_handle;
}

// TODO This code has a really ugly, difficult-to-follow flow.
// If someone ever ends up trying to debug it, don't:
// just refactor it to make it sane.
XRNodeHandle *XRBindNode::open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh)
{
	XUDSHandle *return_handle = NULL;
	XUDSHandle *connect_handle = NULL;
	ConnectHook xuds_hook_proto(NULL);
	void *xuds_hook_rtti = xuds_hook_proto.cheesy_rtti();
	if (xoh->cheesy_rtti() != xuds_hook_rtti)
	{
		// this is open-via-regular-old-open(); give
		// back a handle as if connect()ed to the corresponding name.
		connect_handle = new XUDSHandle(get_xufs());
		return_handle = connect_handle;
	}
	else
	{
		XUDSHook *hook = (XUDSHook *) xoh;
		BindHook *bind_hook = hook->as_bind_hook();
		if (bind_hook!=NULL)
		{
			// This is the open-after-create from xuds_bind().
			// Nothing should be enqueued, and we need to give back
			// a dummy handle that bind can discard (to free
			// the filenum). Something clumsy here.
			// -- bind wants its handle back, and nothing should
			// be enqueued
			connect_handle = NULL;
			return_handle = new XUDSHandle(get_xufs());
		}
		else
		{
			ConnectHook *connect_hook = hook->as_connect_hook();
			lite_assert(connect_hook != NULL);

			// this is open-via-xuds_connect; caller passed a handle in,
			// and doesn't need one back.
			connect_handle = connect_hook->create_connect_handle();
			// but we hand back a fake handle so that caller can close
			// his fd (which calls close() on the handle).
			return_handle = new XUDSHandle(get_xufs());
		}
	}

	if (connect_handle != NULL)
	{
		connect_handle->become_connect(bind_handle);
	}

	*err = XFS_NO_ERROR;
	return return_handle;
}

void XRBindNode::mkdir_node(XfsErr *err, const char *new_name, mode_t mode)
{
	lite_assert(false);	// why would someone DO this!?
	*err = (XfsErr) EINVAL;
}

//////////////////////////////////////////////////////////////////////////////

void XUDSHandle::become_bind(const char *address)
{
	xax_assert(bind_type == xt_unset);
	bind_ep = new XUDSConnectionEndpoint(get_xufs(), address);
	bind_type = xt_bind;
}

void XUDSHandle::become_connect(XUDSHandle *bind_handle)
{
	xax_assert(bind_type == xt_unset);
	connection = new XUDSConnection(bind_handle->bind_ep);
	bind_type = xt_connect_end;
	bind_handle->bind_ep->enqueue(connection);
}

void XUDSHandle::become_accept(XUDSConnection *conn)
{
	xax_assert(bind_type == xt_unset);
	bind_type = xt_accept_end;
	this->connection = conn;
	// We don't addref; we instead inherit responsibility for
	// the ref that was being held by the bind.
	//this->connection->add_ref();
}

XUDSHandle::~XUDSHandle()
{
	delete bind_ep;
	if (connection!=NULL)
	{
		connection->drop_ref(bind_type);
	}
}

void XUDSHandle::_insert_message(XUDSMessage *xum)
{
}

void XUDSHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	*err = (XfsErr) EINVAL;
}

uint32_t XUDSHandle::read(
	XfsErr *err, void *dst, size_t len)
{
	xax_assert(bind_type==xt_connect_end || bind_type==xt_accept_end);
	XUDSMessageEndpoint *read_endpoint = connection->ep[bind_type];

	*err = XFS_NO_ERROR;
	return read_endpoint->consume_part_of_message(dst, len);
}

void XUDSHandle::write(
	XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	lite_assert(offset==0);

	xax_assert(bind_type==xt_connect_end || bind_type==xt_accept_end);
	XUDSMessageEndpoint *write_endpoint = connection->ep[1-bind_type];
	write_endpoint->enqueue(new XUDSMessage(xufs->get_mf(), src, len));
	*err = XFS_NO_ERROR;
}

ZAS *XUDSHandle::get_zas(ZASChannel channel)
{
	if (channel==zas_write)
	{
		// always ready
		return XaxVFSHandlePrototype::get_zas(channel);
	}

	if (bind_type==xt_connect_end || bind_type==xt_accept_end)
	{
		XUDSEndpoint *read_endpoint = connection->ep[bind_type];
		return read_endpoint->get_zas();
	}
	else if (bind_type==xt_bind)
	{
		return bind_ep->get_zas();
	}
	else
	{
		xax_assert(false);
	}
}

XUDSHandle *XUDSHandle::accept_internal()
{
	XUDSConnection *conn = bind_ep->consume();

	XUDSHandle *accept_hdl = new XUDSHandle(get_xufs());
	lite_assert(conn!=NULL);
	accept_hdl->become_accept(conn);
	return accept_hdl;
}

int XUDSHandle::accept(XfsErr *err, XaxVFSHandleTableIfc *xht, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{
	int filenum = xht->allocate_filenum();
	xax_assert(filenum>=0);

	XUDSHandle *accept_hdl = accept_internal();
	xht->assign_filenum(filenum, accept_hdl);

	XfsErr terr;
	accept_hdl->_getepname(&terr, addr, addr_len, xt_accept_end);
	lite_assert(terr==XFS_NO_ERROR);
	
	*err = XFS_NO_ERROR;
	return filenum;
}

static const char *_addr_to_path(ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{
	const struct sockaddr *sa = addr; // addr.__sockaddr__;
	if (sa->sa_family!=AF_UNIX)
	{
		return NULL;
	}

	struct sockaddr_un *sun = (struct sockaddr_un *) sa;
	const char *path = sun->sun_path;

	// man AF_UNIX:
	// "an abstract socket address is distinguished  by  the  fact
	// that  sun_path[0] is a null byte ('\0')"
	if (path[0]=='\0')
	{
		path += 1;
	}
	return path;
}

int XUDSHandle::bind(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{
	const char *path = _addr_to_path(addr, addr_len);
	if (path==NULL)
	{
		return -EINVAL;
	}

	become_bind(path);

	BindHook xoh(this);

	int result = 0;
	int fd = xpe_open_hook_fd(xufs->get_xpe(), path, O_WRONLY|O_CREAT, &xoh);
	if (fd==-1)
	{
		bind_type=xt_broken;
		result = -1;
	}
	else
	{
		xi_close(xufs->get_xpe(), fd);
	}

	*err = XFS_NO_ERROR;
	return result;
}

int XUDSHandle::listen(XfsErr *err, int n)
{
	// we listened at bind() time; sorry.
	*err = XFS_NO_ERROR;
	return 0;
}

int XUDSHandle::recvfrom(XfsErr *err, void *buf, size_t n, int flags, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{
	lite_assert(false); // unimpl
	return -1;
}

int XUDSHandle::recv(XfsErr *err, void *buf, size_t n, int flags)
{
	lite_assert(false); // unimpl
	return -1;
}

int XUDSHandle::recvmsg(XfsErr *err, struct msghdr *message, int flags)
{
	lite_assert(message->msg_name==NULL); // unsupported feature
	lite_assert(message->msg_control==NULL); // unsupported feature
	lite_assert(message->msg_flags==0); // unsupported feature
	lite_assert(message->msg_iov > 0);
	return read(err, message->msg_iov[0].iov_base, message->msg_iov[0].iov_len);
}

int XUDSHandle::send(XfsErr *err, __const __ptr_t buf, size_t n, int flags)
{
	lite_assert(false); // unimpl
	return -1;
}

int XUDSHandle::sendmsg(XfsErr *err, const struct msghdr *message, int flags)
{
	lite_assert(false); // unimpl
	return -1;
}

int XUDSHandle::sendto(XfsErr *err, __const __ptr_t buf, size_t n, int flags, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{
	lite_assert(false); // unimpl
	return -1;
}

int XUDSHandle::getsockopt(XfsErr *err, int level, int optname, void *optval, socklen_t *optlen)
{
	if (level==SOL_SOCKET)
	{
		switch (optname)
		{
		case SO_PEERCRED:
		{
			struct ucred *cr = (struct ucred *) optval;
			lite_assert(sizeof(*cr)==*optlen);
			get_xufs()->fill_ucred(cr);
			*err = XFS_NO_ERROR;
			return 0;
		}
		default:
			/*fall through*/;
		}
	}
	*err = (XfsErr) EINVAL;
	return -1;
}

int XUDSHandle::setsockopt(XfsErr *err, int level, int optname, const __ptr_t optval, socklen_t optlen)
{
	// don't need SO_REUSEADDR here; I think clients only call it for IP sockets
	*err = (XfsErr) EINVAL;
	return -1;
}

int XUDSHandle::shutdown(XfsErr *err, int how)
{
	// just lying for now; hard to imagine an app that would care that
	// we left it open.
	*err = XFS_NO_ERROR;
	return 0;
}

int XUDSHandle::connect(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{
	const char *path = _addr_to_path(addr, addr_len);
	if (path==NULL)
	{
		*err = (XfsErr) EINVAL;
		return -1;
	}

	// pass my handle in, via hook, to open, where it'll get enqueued
	// on the xt_bind handle's queue by xuds_bind_open().
	ConnectHook xoh(this);

	XVFSHandleWrapper *w = xpe_open_hook_handle(xufs->get_xpe(), err, path, O_RDWR, &xoh);
	if (*err!=XFS_NO_ERROR)
	{
		return -1;	// pass through err
	}

	// Just opening got me queued into the bind queue.

	// close the dummy handle I got back.
	w->drop_ref();
	xax_assert(*err == XFS_NO_ERROR);

	return 0;
}

int XUDSHandle::_getepname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len, XUDSEndpointType whichep)
{
	if (bind_type==xt_bind)
	{
		lite_assert(false);	// case not implemented yet
	}

	if (bind_type!=xt_accept_end
		&& bind_type!=xt_connect_end)
	{
		xax_assert(0);	// why is someone calling this at a bad time?
		*err = (XfsErr) EINVAL;
		return -1;
	}

	XUDSEndpoint *ep = connection->ep[whichep];
	sockaddr_un *un = (sockaddr_un *) addr;
	uint32_t addr_size = sizeof(un->sun_family)+lite_strlen(ep->get_address());
	memset(un, 0, *addr_len);	// zero-terminate where feasible, just as a debugging courtesy.
	// The  returned address is truncated if the buffer provided is too small;
    // in this case, addrlen will return a value greater than was supplied  to
    // the call.
	uint32_t copy_amount = min(*addr_len, addr_size);
	un->sun_family = AF_UNIX;
	memcpy(un->sun_path, ep->get_address(), copy_amount-sizeof(un->sun_family));
	int rc;
	if (copy_amount == addr_size)
	{
		rc = 0;
	}
	else
	{
		// hint to allocate more space
		rc = addr_size;
	}

	*addr_len = addr_size;
	*err = XFS_NO_ERROR;
	return rc;
}

int XUDSHandle::getpeername(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{
	return _getepname(err, addr, addr_len, (XUDSEndpointType) (1-bind_type));
}

int XUDSHandle::getsockname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{
	return _getepname(err, addr, addr_len, bind_type);
}

XUDSFileSystem::XUDSFileSystem(XaxPosixEmulation *xpe)
{
	this->xpe = xpe;
	this->mf = xpe->mf;
}

XaxVFSHandleIfc *XUDSFileSystem::open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks)
{
	// this fs is only mounted under _socketfactory, and
	// hence only ever has its socketset() interface called.
	lite_assert(false);
	return NULL;
}

void XUDSFileSystem::socketset(XaxVFSHandleTableIfc *xht, XaxVFSIfc *xfs, XfsErr *err, int domain, int type, int protocol, XaxVFSHandleIfc **out_handles, int n)
{
	if (n==1)
	{
		out_handles[0] = new XUDSHandle(this);
			// We don't know yet if this handle will be bind()ed or connect()ed.
			// so its type is xt_unset.

		*err = XFS_NO_ERROR;
	}
	else if (n==2)
	{
		// TODO we leak this bind handle.
		XUDSHandle *bind_handle = new XUDSHandle(this);
		bind_handle->become_bind("");

		XUDSHandle *connect_handle = new XUDSHandle(this);
		connect_handle->become_connect(bind_handle);
		out_handles[0] = connect_handle;

		XUDSHandle *accept_handle = bind_handle->accept_internal();
		out_handles[1] = accept_handle;
		*err = XFS_NO_ERROR;
	}
	else
	{
		lite_assert(false);	// n>2?
	}
}

void XUDSFileSystem::fill_ucred(struct ucred *cr)
{
	cr->pid = xpe->fake_ids.pid;
	cr->uid = xpe->fake_ids.uid;
	cr->gid = xpe->fake_ids.gid;
}

void xax_unixdomainsockets_init(XaxPosixEmulation *xpe)
{
	char pathbuf[1024];
	lite_strcpy(pathbuf, "/_socketfactory/");
	char hexbuf[10];
	lite_strcat(pathbuf, hexlong(hexbuf, AF_UNIX));

	XUDSFileSystem *xufs = new XUDSFileSystem(xpe);
	xpe_mount(xpe, mf_strdup(xpe->mf, pathbuf), xufs);
}
