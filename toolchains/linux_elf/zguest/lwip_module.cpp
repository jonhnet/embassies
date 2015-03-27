#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

typedef __off64_t off64_t;
#define weak_function /**/
#include "pal_abi/pal_abi.h"
#include "XaxVFS.h"

#include "xax_extensions.h"

//#include "lwip/sockets.h"

#include "namespace_insulator.h"
#include "invoke_debugger.h"

typedef struct sockaddr unix_sockaddr;
typedef struct sockaddr_in unix_sockaddr_in;
#undef ZSOCKADDR_ARG
#define ZSOCKADDR_ARG unix_sockaddr *
#undef ZCONST_SOCKADDR_ARG
#define ZCONST_SOCKADDR_ARG __const unix_sockaddr *
#define sockaddr	KABOOOM
#define sockaddr_in	KABOOOM

extern "C" {
#include "lwip/api.h"
#include "xaxif.h"
}
#include "lwip_module.h"

#include "assert.h"
#include "concrete_mmap_xdt.h"

#define LWIP_ERRNO (EINVAL)
	// lwip isn't presently setting gs errno, which is good, since we'd
	// like not to touch gs TLS. Return the same error code for
	// every error, and if that doesn't work out, we'll figure out
	// another way to patch it up; eg by passing a per-call out parameter
	// a la XfsErr (and patching lwip sockets.c to support that).

#define EXTRACT_LWIP_ERRNO_WHEN(errorwhen)	((XfsErr)((errorwhen)?(LWIP_ERRNO):XFS_NO_ERROR))
	// TODO How can EXTRACT_LWIP_ERRNO_WHEN possibly work when it's compiled in
	// a different domain? Why aren't we blowing up gs all over the place?

//////////////////////////////////////////////////////////////////////////////

#define MAKE_ALIGNED_LWIP_SOCKADDR() \
	((struct lwip_sockaddr*) \
		(((unsigned int)(((uint8_t*)alloca(sizeof(unix_sockaddr)+3))+3)) & ~3))
void unix_to_lwip_sockaddr(struct lwip_sockaddr *lwip_sockaddr, const unix_sockaddr *unix_addr, int unix_addr_len)
{
	struct lwip_sockaddr_in *lwip_si = (struct lwip_sockaddr_in *) lwip_sockaddr;
	lwip_si->sin_len	= sizeof(struct lwip_sockaddr_in);
	assert(unix_addr->sa_family == AF_INET);
	unix_sockaddr_in *sin = (unix_sockaddr_in *)unix_addr;
	lwip_si->sin_family	= sin->sin_family;
	lwip_si->sin_port	= sin->sin_port;
	lwip_si->sin_addr	= sin->sin_addr;
}

void lwip_to_unix_sockaddr(struct lwip_sockaddr *lwip_addr, unix_sockaddr *unix_addr, unsigned int *unix_addr_len)
{
	struct lwip_sockaddr_in *lwip_si = (struct lwip_sockaddr_in *) lwip_addr;
	assert(lwip_addr->sa_family == AF_INET);
	unix_addr->sa_family = AF_INET;
	unix_sockaddr_in *sin = (unix_sockaddr_in *)unix_addr;
	sin->sin_family	= lwip_si->sin_family;
	sin->sin_port		= lwip_si->sin_port;
	sin->sin_addr		= lwip_si->sin_addr;
	assert(sizeof(*unix_addr) <= *unix_addr_len);
	*unix_addr_len = sizeof(*unix_addr);
}

//////////////////////////////////////////////////////////////////////////////

class LWIPFS;

class LWIPHandle : public XaxVFSHandlePrototype, public XaxVirtualSocketIfc
{
public:
	LWIPHandle(LWIPFS *lwipfs, int lwipfd);
	~LWIPHandle();

	virtual bool is_stream() { return true; }
	virtual uint64_t get_file_len() { lite_assert(false); return 0; }

	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);

	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);

	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);

	virtual ZAS *get_zas(
		ZASChannel channel);

	virtual XaxVirtualSocketIfc *as_socket() { return this; }

	virtual int accept(XfsErr *err, XaxVFSHandleTableIfc *xht, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	virtual int bind(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	virtual int connect(XfsErr *err, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	virtual int getpeername(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	virtual int getsockname(XfsErr *err, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	virtual int getsockopt(XfsErr *err, int level, int optname, void *optval, socklen_t *optlen);
	virtual int listen(XfsErr *err, int n);
	virtual int recvfrom(XfsErr *err, void *buf, size_t n, int flags, ZSOCKADDR_ARG addr, socklen_t *addr_len);
	virtual int recv(XfsErr *err, void *buf, size_t n, int flags);
	virtual int recvmsg(XfsErr *err, struct msghdr *message, int flags);
	virtual int send(XfsErr *err, __const __ptr_t buf, size_t n, int flags);
	virtual int sendmsg(XfsErr *err, const struct msghdr *message, int flags);
	virtual int sendto(XfsErr *err, __const __ptr_t buf, size_t n, int flags, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len);
	virtual int setsockopt(XfsErr *err, int level, int optname, const __ptr_t optval, socklen_t optlen);
	virtual int shutdown(XfsErr *err, int how);

private:
	int lwipfd;	// the name given to this handle by lwip sockets-compatible api
	LWIPFS *lwipfs;
	bool vnc_sync_bind_X;	// expedient to detect when Xvnc opens its socket
};

class LWIPFS : public XaxVFSPrototype, public XaxVirtualSocketProvider
{
public:
	LWIPFS(ZoogDispatchTable_v1 *zdt, XvncStartDetector *xsd);

	virtual XaxVFSHandleIfc *open(
		XfsErr *err,
		XfsPath *path,
		int oflag,
		XVOpenHooks *open_hooks = NULL);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

	virtual XaxVirtualSocketProvider *as_socket_provider() { return this; }

	virtual void socketset(XaxVFSHandleTableIfc *xht, XaxVFSIfc *xfs, XfsErr *err, int domain, int type, int protocol, XaxVFSHandleIfc **out_handles, int n);

	ZAS *get_zas() { return &zar.zas; }

	void xvnc_start_signal() { xsd->signal(); }

private:
	ZoogDispatchTable_v1 *zdt;
	XvncStartDetector *xsd;
	ZAlwaysReady zar;
};

//////////////////////////////////////////////////////////////////////////////

LWIPHandle::LWIPHandle(LWIPFS *lwipfs, int lwipfd)
{
	this->lwipfd = lwipfd;
	this->lwipfs = lwipfs;
	this->vnc_sync_bind_X = false;
}

LWIPHandle::~LWIPHandle()
{
	lwip_close(lwipfd);
}

uint32_t LWIPHandle::read(XfsErr *err, void *dst, size_t len)
{
	int rc = lwip_read(lwipfd, dst, len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

void LWIPHandle::write(XfsErr *err, const void *src, size_t len, uint64_t offset)
{
	lite_assert(offset==0);	// stream version
	int rc = lwip_write(lwipfd, src, len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
}

void LWIPHandle::xvfs_fstat64(
	XfsErr *err, struct stat64 *buf)
{
	*err = (XfsErr) EINVAL;
}

ZAS *LWIPHandle::get_zas(ZASChannel channel)
{
	switch (channel)
	{
		case zas_read:
		{
			return lwip_get_read_zas(lwipfd);
		}
		case zas_write:
			return lwipfs->get_zas();
		case zas_except:
			return lwipfs->get_zas();
		default:
			assert(0);	// bogus param
	}
}

int LWIPHandle::accept(XfsErr *err, XaxVFSHandleTableIfc *xht, ZSOCKADDR_ARG unix_addr, socklen_t *unix_addr_len)
{
	struct lwip_sockaddr *lwip_addr = MAKE_ALIGNED_LWIP_SOCKADDR();
	socklen_t lwip_len = sizeof(struct lwip_sockaddr);

	int new_lwipfd = lwip_accept(lwipfd, lwip_addr, &lwip_len);
	int result;
	if (new_lwipfd>=0)
	{
		LWIPHandle *handle = new LWIPHandle(lwipfs, new_lwipfd);
		int xpe_filenum = xht->allocate_filenum();
		if (xpe_filenum == -1)
		{
			lite_assert(0);	// TODO need to free handle
			*err = (XfsErr) EMFILE;
			result = -1;
		}
		else
		{
			if (unix_addr!=NULL)
			{
				lwip_to_unix_sockaddr(lwip_addr, unix_addr, unix_addr_len);
			}

			xht->assign_filenum(xpe_filenum, handle);
			*err = XFS_NO_ERROR;
			result = xpe_filenum;
		}
	}
	else
	{
		result = new_lwipfd;
	}
	*err = EXTRACT_LWIP_ERRNO_WHEN(result<0);
	return result;
}

int LWIPHandle::bind(XfsErr *err, ZCONST_SOCKADDR_ARG unix_addr, socklen_t unix_addr_len)
{
	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	unix_to_lwip_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);

	int lwiprc = lwip_bind(lwipfd, lwip_sockaddr, lwip_sockaddr->sa_len);

	// disgusting vnc snoopery
	if (lwiprc==0)
	{
		// I can't figure out where frickin' 'struct sockaddr' is defined.
		// It's in some include files, as long as you're not GNU or GLIBC or
		// what have you. I give up; I'm going to cheat.
		struct lwip_sockaddr_in *saddr = (struct lwip_sockaddr_in *)lwip_sockaddr;
		if (saddr->sin_family == AF_INET)
		{
			// weird, I'd have guessed this field would be in host order.
			uint16_t port = ntohs(saddr->sin_port);
			if (port >= 6000 && port<6100)
			{
				vnc_sync_bind_X = true;
			}
		}
	}

	*err = EXTRACT_LWIP_ERRNO_WHEN(lwiprc<0);
	return lwiprc;
}

int LWIPHandle::connect(XfsErr *err, ZCONST_SOCKADDR_ARG unix_addr, socklen_t unix_addr_len)
{
	if (unix_addr->sa_family==AF_UNSPEC)
	{
		// UNSPEC: weird expedient deal with "reset the connection" in libc's
		// sysdeps/posix/getaddrinfo.c:2234
		*err = (XfsErr) ENOSYS;
		return -1;
	}

	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	unix_to_lwip_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);

	int rc = lwip_connect(lwipfd, lwip_sockaddr, lwip_sockaddr->sa_len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::getpeername(XfsErr *err, ZSOCKADDR_ARG unix_addr, socklen_t *unix_addr_len)
{
	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	socklen_t lwip_socklen = sizeof(*lwip_sockaddr);
	int rc = lwip_getpeername(lwipfd, lwip_sockaddr, &lwip_socklen);
	lwip_to_unix_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::getsockname(XfsErr *err, ZSOCKADDR_ARG unix_addr, socklen_t *unix_addr_len)
{
	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	socklen_t lwip_socklen = sizeof(*lwip_sockaddr);
	int rc = lwip_getsockname(lwipfd, lwip_sockaddr, &lwip_socklen);
	lwip_to_unix_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::getsockopt(XfsErr *err, int level, int optname, void *optval, socklen_t *optlen)
{
    // Translate between POSIX symbol definitions and LWIP's "symbol definitions"
    if (level == SOL_SOCKET)
	{
        level = 0xfff;
    }
    else
	{
        *err = (XfsErr) ENOSYS;
        return -1;
    }

    if (optname == SO_ERROR)
	{
        optname = 0x1007;
    }
    else
	{
        *err = (XfsErr) ENOSYS;
        return -1;
    }
        
    int rc = lwip_getsockopt(lwipfd, level, optname, optval, optlen);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
    return rc;
}


int LWIPHandle::listen(XfsErr *err, int backlog)
{
	int rc = lwip_listen(lwipfd, backlog);
	if (rc==0)
	{
		if (vnc_sync_bind_X)
		{
			lwipfs->xvnc_start_signal();
		}
	}

	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::recv(XfsErr *err, void *buf, size_t n, int flags)
{
	int rc = lwip_recv(lwipfd, buf, n, flags);
	*err = XFS_NO_ERROR;
	return rc;
}

int LWIPHandle::recvfrom(XfsErr *err, void *buf, size_t n, int flags, ZSOCKADDR_ARG unix_addr, socklen_t *unix_addr_len)
{
	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	socklen_t lwip_socklen = sizeof(*lwip_sockaddr);
	int rc = lwip_recvfrom(lwipfd, buf, n, flags, lwip_sockaddr, &lwip_socklen);
	lwip_to_unix_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::recvmsg(XfsErr *err, struct msghdr *message, int flags)
{
	lite_assert(false);	// unimpl
	return -1;
}

int LWIPHandle::send(XfsErr *err, __const __ptr_t buf, size_t n, int flags)
{
	int rc = lwip_send(lwipfd, buf, n, flags);
	*err = XFS_NO_ERROR;
	return rc;
}

int LWIPHandle::sendmsg(XfsErr *err, const struct msghdr *message, int flags)
{
	lite_assert(false);	// unimpl
	return -1;
}

int LWIPHandle::sendto(XfsErr *err, __const __ptr_t buf, size_t n, int flags, ZCONST_SOCKADDR_ARG unix_addr, socklen_t unix_addr_len)
{
	struct lwip_sockaddr *lwip_sockaddr = MAKE_ALIGNED_LWIP_SOCKADDR();
	unix_to_lwip_sockaddr(lwip_sockaddr, unix_addr, unix_addr_len);
	int rc = lwip_sendto(lwipfd, buf, n, flags, lwip_sockaddr, lwip_sockaddr->sa_len);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

int LWIPHandle::setsockopt(XfsErr *err, int level, int optname, const __ptr_t optval, socklen_t optlen)
{
#if 0
	LWIPHandle *lh = (LWIPHandle *) xh;
    int rc = lwip_setsockopt(lh->lwipfd, level, optname, optval, optlen);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
    return rc;
#endif

	//LWIPHandle *h = (LWIPHandle *) xh;

#define UNIX_SOL_SOCKET 1
#define UNIX_SO_REUSEADDR 2
	if (level==UNIX_SOL_SOCKET && optname==UNIX_SO_REUSEADDR)
	{
		*err = XFS_NO_ERROR;
		return 0;
	}

	*err = (XfsErr) ENOSYS;
	return -1;
}

int LWIPHandle::shutdown(XfsErr *err, int how)
{
	int rc = lwip_shutdown(lwipfd, how);
	*err = EXTRACT_LWIP_ERRNO_WHEN(rc<0);
	return rc;
}

//////////////////////////////////////////////////////////////////////////////

LWIPFS::LWIPFS(ZoogDispatchTable_v1 *zdt, XvncStartDetector *xsd)
{
	this->zdt = zdt;
	this->xsd = xsd;
	zar_init(&zar);
}

XaxVFSHandleIfc *LWIPFS::open(
	XfsErr *err,
	XfsPath *path,
	int oflag,
	XVOpenHooks *open_hooks)
{
	lite_assert(false);
	return NULL;
}

void LWIPFS::stat64(
	XfsErr *err,
	XfsPath *path, struct stat64 *buf)
{
	*err = (XfsErr) EINVAL;
}

void LWIPFS::socketset(XaxVFSHandleTableIfc *xht, XaxVFSIfc *xfs, XfsErr *err, int domain, int type, int protocol, XaxVFSHandleIfc **out_handles, int n)
{
	assert(n==1);	// todo: implement socketpair
	int lwipfd = lwip_socket(domain, type, protocol);
	if (lwipfd<0)
	{
		*err = EXTRACT_LWIP_ERRNO_WHEN(1);
		return;
	}

	out_handles[0] = new LWIPHandle(this, lwipfd);
	*err = XFS_NO_ERROR;
}

#if 0
void lm_llseek(XfsErr *err, XfsHandle *h, int64_t offset, int whence, int64_t *retval)
{
	*err = ESPIPE;
}
#endif

void setup_lwip(ZoogDispatchTable_v1 *zdt, XvncStartDetector *xsd)
{
	LWIPFS *lwipfs = new LWIPFS(zdt,xsd);

	xaxif_setup(zdt);

	// Ugh. If malloc is involved, and hence TLS, this will cause a world
	// of despair. :v(
	// Thankfully, malloc is factored out of lwip.
	// Look in ports/unix/sys_arch.c and ports/unix/netif/*.c for some
	// direct mallocs (rather than lwip mem_mallocs) that will need fixin'.

	// Oh drat. I think I just brought ambient malloc back in, by way
	// of new. Well, at least it's operator new wired to my malloc_factory.
	// I hope.

	char pathbuf[1024];
	sprintf(pathbuf, "/_socketfactory/%08x", AF_INET);

	xe_xpe_mount_vfs(strdup(pathbuf), lwipfs);
}

// LWIP seems to need this declared somewhere:
unsigned char debug_flags;
