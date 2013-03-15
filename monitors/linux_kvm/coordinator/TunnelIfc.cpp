#include <alloca.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#if DEBUG_CAPABILITIES
#include <sys/capability.h>
#endif // DEBUG_CAPABILITIES

#include "LiteLib.h"
#include "TunnelIfc.h"
#include "Router.h"
#include "AppLink.h"
#include "MonitorPacket.h"

typedef struct
{
	struct tun_pi tpi;
	char data[];
} TunFrame;

static void xip_to_sockaddr(struct sockaddr_in *sin, XIPAddr *addr)
{
	switch (addr->version)
	{
	case ipv4:
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = get_ip4(addr);
		break;
	case ipv6:
		sin->sin_family = AF_INET;
		lite_assert(false);
		//sin->sin_addr.s_addr = get_ip6(addr);
		break;
	default:
		lite_assert(false);
	}
}

TunnelIfc::TunnelIfc(Router *router, AppLink *app_link, TunIDAllocator* tunid)
{
	this->router = router;	// where we hand off incoming packets

	const char *tun_device = "/dev/net/tun";
	tun_fd = open(tun_device, O_RDWR);
	lite_assert(tun_fd>=0);

	int rc;
	rc = fcntl(tun_fd, F_SETFD, FD_CLOEXEC);
	if (rc!=0)
	{
		fprintf(stderr, "Cannot open %s.\n"
			"The most likely cause of this is that you aren't running with\n"
			"permissions to open the device.\n",
			tun_device);
		lite_assert(rc==0);
	}
	
	int ioctlsocket = tunid->open_tunnel();

	struct ifreq ifr;
	tunid->setup_ifreq(&ifr);
	
	rc = ioctl(ioctlsocket, SIOCGIFFLAGS, &ifr);
	lite_assert(rc==0);

	ifr.ifr_ifru.ifru_flags |= IFF_UP;
	rc = ioctl(ioctlsocket, SIOCSIFFLAGS, &ifr);
	lite_assert(rc==0);

	ifr.ifr_mtu = 65535;
	rc = ioctl(ioctlsocket, SIOCSIFMTU, &ifr);
	lite_assert(rc==0);

	struct sockaddr_in *sin;
	sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr;
	XIPAddr gateway = app_link->get_gateway(ipv4);
	xip_to_sockaddr(sin, &gateway);
	rc = ioctl(ioctlsocket, SIOCSIFADDR, &ifr);
	lite_assert(rc==0);

	sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_netmask;
	XIPAddr netmask = app_link->get_netmask(ipv4);
	xip_to_sockaddr(sin, &netmask);
	rc = ioctl(ioctlsocket, SIOCSIFNETMASK, &ifr);
	lite_assert(rc==0);

	sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_dstaddr;
	XIPAddr subnet = app_link->get_subnet(ipv4);
	xip_to_sockaddr(sin, &subnet);
	rc = ioctl(ioctlsocket, SIOCSIFDSTADDR, &ifr);	// route this subnet here
	lite_assert(rc==0);

	pthread_create(&recv_thread, NULL, _recv_thread, this);
}

void *TunnelIfc::_recv_thread(void *v_this)
{
	((TunnelIfc*) v_this)->_recv_thread();
	lite_assert(false);	// notreached
	return NULL;
}

void TunnelIfc::_recv_thread()
{
	int recv_size;
	int max_packet_size = 65536;
	char *buffer = (char*) alloca(max_packet_size);
	lite_assert(buffer!=NULL);
	while (1)
	{
		recv_size = read(tun_fd, buffer, max_packet_size);
		if (recv_size<=0)
		{
			perror("read");
			lite_assert(0);
		}

		TunFrame *frame = (TunFrame*) buffer;
		uint32_t iplen = recv_size - sizeof(TunFrame);

		IPInfo info;
		decode_ip_packet(&info, frame->data, iplen);
		lite_assert(info.packet_valid);
		lite_assert(iplen == info.packet_length);

		Message *m = new Message(frame->data, iplen);
		Packet *p = new MonitorPacket(m);
		// Packet *p = new TunnelPacket(frame->data, iplen);
		router->deliver_packet(p);
	}
}

void TunnelIfc::deliver(Packet *p)
{
	if (p->get_size() >= 1<<20)
	{
		fprintf(stderr, "packet > 1MB; dropping on tunnel outbound link.\n");
	}
	else
	{
		int framelen = sizeof(TunFrame) + p->get_size();
		TunFrame *frame = (TunFrame *) alloca(framelen);
		frame->tpi.flags = 0;
		frame->tpi.proto = 8;	 // IP protocol (what's the symbolic name for that?)
		memcpy(frame->data, p->get_payload(), p->get_size());

		int rc = write(tun_fd, frame, framelen);
		if (rc!=framelen)
		{
			perror("write");
		}
		lite_assert(rc==framelen);
	}

	delete p;
}

XIPAddr TunnelIfc::get_subnet(XIPVer ver)
{
	return xip_all_zeros(ver);
}

XIPAddr TunnelIfc::get_netmask(XIPVer ver)
{
	return xip_all_zeros(ver);
}
