#include <assert.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif // _WIN32

#include "sockaddr_util.h"

void sockaddr_from_ep(struct sockaddr *sa, size_t sa_size, UDPEndpoint *ep)
{
	assert(sa_size >= sizeof(struct sockaddr_in));
	assert(ep->ipaddr.version == ipv4);	// code not generalized to ipv6 yet
	struct sockaddr_in *si = (struct sockaddr_in *) sa;
	si->sin_family = PF_INET;
	si->sin_addr.s_addr = ep->ipaddr.xip_addr_un.xv4_addr;
	si->sin_port = ep->port;
}

void ep_from_sockaddr(UDPEndpoint *ep, struct sockaddr *sa, size_t sa_size)
{
	assert(sa_size >= sizeof(struct sockaddr_in));
	struct sockaddr_in *si = (struct sockaddr_in *) sa;
	assert(si->sin_family == PF_INET);	// code not generalized to ipv6 yet
	ep->ipaddr.version = ipv4;
	ep->ipaddr.xip_addr_un.xv4_addr = si->sin_addr.s_addr;
	ep->port = si->sin_port;
}

