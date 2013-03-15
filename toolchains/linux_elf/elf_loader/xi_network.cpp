#include <errno.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/types.h>
//#include <sys/socket.h>

#include "LiteLib.h"
#include "debug_util.h"
#include "xax_posix_emulation.h"
#include "xpe_route_macros.h"
#include "xi_network.h"
#include "cheesy_snprintf.h"
// #include "xax_skinny_network.h"
#include "zsockaddr.h"
#include "XVFSHandleWrapper.h"
#include "xax_util.h"

int xi_accept(XaxPosixEmulation *xpe, int filenum, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{	ROUTE_NONBLOCKING(as_socket()->accept, zas_read, xpe->handle_table, addr, addr_len); }

int xi_bind(XaxPosixEmulation *xpe, int filenum, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{	ROUTE_FCN(as_socket()->bind, addr, addr_len); }

int xi_connect(XaxPosixEmulation *xpe, int filenum, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{	ROUTE_FCN(as_socket()->connect, addr, addr_len); }

int xi_getpeername(XaxPosixEmulation *xpe, int filenum, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{	ROUTE_FCN(as_socket()->getpeername, addr, addr_len); }

int xi_getsockname(XaxPosixEmulation *xpe, int filenum, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{	ROUTE_FCN(as_socket()->getsockname, addr, addr_len); }

int xi_getsockopt(XaxPosixEmulation *xpe, int filenum, int level, int optname, void *optval, socklen_t *optlen)
{	ROUTE_FCN(as_socket()->getsockopt, level, optname, optval, optlen); }

int xi_listen(XaxPosixEmulation *xpe, int filenum, int n)
{	ROUTE_FCN(as_socket()->listen, n); }

int xi_recvfrom(XaxPosixEmulation *xpe, int filenum, void *buf, size_t n, int flags, ZSOCKADDR_ARG addr, socklen_t *addr_len)
{	ROUTE_FCN(as_socket()->recvfrom, buf, n, flags, addr, addr_len); }

int xi_recvmsg(XaxPosixEmulation *xpe, int filenum, struct msghdr *message, int flags)
{	ROUTE_FCN(as_socket()->recvmsg, message, flags); }

int xi_recv(XaxPosixEmulation *xpe, int filenum, void *buf, size_t n, int flags)
{	ROUTE_FCN(as_socket()->recv, buf, n, flags); }

int xi_sendto(XaxPosixEmulation *xpe, int filenum, const void * buf, size_t n, int flags, ZCONST_SOCKADDR_ARG addr, socklen_t addr_len)
{	ROUTE_FCN(as_socket()->sendto, buf, n, flags, addr, addr_len); }

int xi_sendmsg(XaxPosixEmulation *xpe, int filenum, const struct msghdr *message, int flags)
{	ROUTE_FCN(as_socket()->sendmsg, message, flags); }

int xi_send(XaxPosixEmulation *xpe, int filenum, const void * buf, size_t n, int flags)
{	ROUTE_FCN(as_socket()->send, buf, n, flags); }

int xi_setsockopt(XaxPosixEmulation *xpe, int filenum, int level, int optname, const void * optval, socklen_t optlen)
{	ROUTE_FCN(as_socket()->setsockopt, level, optname, optval, optlen); }

int xi_shutdown(XaxPosixEmulation *xpe, int filenum, int how)
{	ROUTE_FCN(as_socket()->shutdown, how); }

#define SOCK_NONBLOCK (0x800)
int _xi_socketset(XaxPosixEmulation *xpe, XaxVFSHandleTableIfc *xht, int domain, int type, int protocol, int *fds, int n)
{
	// TODO: expedient
	bool nonblock = false;
	if ((type & SOCK_NONBLOCK) > 0) {
		nonblock = true;
		type &= ~SOCK_NONBLOCK;
	}
	char pathbuf[100];
	pathbuf[0] = '\0';
	lite_strcat(pathbuf, "/_socketfactory/");
	char hexbuf[10];
	lite_strcat(pathbuf, hexlong(hexbuf, domain));
	lite_strcat(pathbuf, "/");
	lite_strcat(pathbuf, hexlong(hexbuf, type));
	lite_strcat(pathbuf, "/");
	lite_strcat(pathbuf, hexlong(hexbuf, protocol));

#if 0
	_xaxunder___write(2, pathbuf, strlen(pathbuf));
	_xaxunder___write(2, "\n", 1);
#endif

	XfsPath xpath;
	XVFSMount *mount = _xpe_map_path_to_mount(xpe, pathbuf, &xpath);
	if (mount==NULL || mount->xfs->as_socket_provider()==NULL)
	{
		return -ENOSYS;	// no socket manager registered
	}

	XfsErr err = XFS_DIDNT_SET_ERROR;
	XaxVFSHandleIfc *handles[2];
	handles[0] = NULL;	// lest some buggy xfs (cough cough lwip_module) incorrectly set *err==XFS_NO_ERROR without assigning handles.
	handles[1] = NULL;
	xax_assert(1<=n);
	xax_assert(n<=2);
	mount->xfs->as_socket_provider()->socketset(
		xpe->handle_table, mount->xfs, &err, domain, type, protocol, handles, n);
	lite_assert(err!=XFS_DIDNT_SET_ERROR);
	if (err!=XFS_NO_ERROR)
	{
		_xpe_path_free(&xpath);
		return -err;
	}

	int ni;
	for (ni=0; ni<n; ni++)
	{
		int filenum = xht->allocate_filenum();
		if (filenum == -1)
		{
			goto explode;
		}
		fds[ni] = filenum;
	}

	for (ni=0; ni<n; ni++)
	{
		xht->assign_filenum(fds[ni], handles[ni]);

		if (nonblock) {
			xht->set_nonblock(fds[ni]);
		}
	}

	_xpe_path_free(&xpath);
	return 0;

explode:
	for (ni=0; ni<n; ni++)
	{
		delete handles[ni];
		xht->free_filenum(fds[ni]);
		fds[ni] = -1;
	}
	_xpe_path_free(&xpath);
	return -EMFILE;
}

int xi_socket(XaxPosixEmulation *xpe, int domain, int type, int protocol)
{
	int fds[1];
	int rc = _xi_socketset(xpe, xpe->handle_table, domain, type, protocol, fds, 1);
	return (rc==0) ? fds[0] : -1;
}

int xi_socketpair(XaxPosixEmulation *xpe, int domain, int type, int protocol, int fds[2])
{
	return _xi_socketset(xpe, xpe->handle_table, domain, type, protocol, fds, 2);
}

int xi_socketcall(XaxPosixEmulation *xpe, int call, SocketcallArgs *args)
{
	switch (call)
	{
	case SYS_SOCKET:
		return xi_socket(xpe, args[0], args[1], args[2]);
	case SYS_BIND:
		return xi_bind(xpe, args[0], (ZCONST_SOCKADDR_ARG) args[1], args[2]);
	case SYS_CONNECT:
		return xi_connect(xpe, args[0], (ZCONST_SOCKADDR_ARG) args[1], args[2]);
	case SYS_LISTEN:
		return xi_listen(xpe, args[0], args[1]);
	case SYS_ACCEPT:
		return xi_accept(xpe, args[0], (ZSOCKADDR_ARG) args[1], (socklen_t*) args[2]);
	case SYS_GETSOCKNAME:
		return xi_getsockname(xpe, args[0], (ZSOCKADDR_ARG) args[1], (socklen_t*) args[2]);
	case SYS_GETPEERNAME:
		return xi_getpeername(xpe, args[0], (ZSOCKADDR_ARG) args[1], (socklen_t*) args[2]);
	case SYS_SOCKETPAIR:
		return xi_socketpair(xpe, args[0], args[1], args[2], (int*) (args[3]));
	case SYS_SEND:
		return xi_send(xpe, args[0], (const void *) args[1], args[2], args[3]);
	case SYS_RECV:
		return xi_recv(xpe, args[0], (void *) args[1], args[2], args[3]);
	case SYS_SENDTO:
		return xi_sendto(xpe, args[0], (const void *) args[1], args[2], args[3], (ZCONST_SOCKADDR_ARG) args[4], args[5]);
	case SYS_RECVFROM:
		return xi_recvfrom(xpe, args[0], (void *) args[1], args[2], args[3], (ZSOCKADDR_ARG) args[4], (socklen_t*) args[5]);
	case SYS_SHUTDOWN:
		return xi_shutdown(xpe, args[0], args[1]);
	case SYS_SETSOCKOPT:
		return xi_setsockopt(xpe, args[0], args[1], args[2], (const void *) args[3], args[4]);
	case SYS_GETSOCKOPT:
		return xi_getsockopt(xpe, args[0], args[1], args[2], (void *) args[3], (socklen_t *) args[4]);
	case SYS_SENDMSG:
		return xi_sendmsg(xpe, args[0], (const struct msghdr *) args[1], args[2]);
	case SYS_RECVMSG:
		return xi_recvmsg(xpe, args[0], (struct msghdr *) args[1], args[2]);
#ifdef SYS_ACCEPT4
	case SYS_ACCEPT4:
#endif // SYS_ACCEPT4
	default:
		xax_assert(0);	// unimplemented
		return -ENOSYS;

	}
}
