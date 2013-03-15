#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <malloc.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Monitor.h"
#include "netifc.h"
#include "linux_dbg_port_protocol.h"
#include "xax_network_utils.h"
#include "format_ip.h"
#include "forwarding_table.h"
#include "simple_network.h"

void send_packet(NetIfc *ni, IPInfo *info, PacketIfc *p);

typedef struct
{
	struct tun_pi tpi;
	char data[];
} TunFrame;

void print_time(const char *msg, int txnum);
void *ni_read_thread(void *vni);
void _ni_init_forwarding_table(NetIfc *ni, MallocFactory *mf);

void _netifc_deliver_tunnel(NetIfc *ni, PacketIfc *p, XIPAddr *addr);
void _netifc_deliver_local(NetIfc *ni, PacketIfc *p, XIPAddr *dest);
void _netifc_deliver_broadcast(NetIfc *ni, PacketIfc *p, XIPAddr *dest);

void _net_connection_init(NetConnection *nconn, NetIfc *ni, XIP_ifc_config *ifconfig, int app_index, PacketQueue *pq, AppInterface* ai);
void _net_connection_free(NetConnection *nconn, NetIfc *ni);
int _net_connection_allocate_index(NetIfc *ni);

void xip_to_sockaddr(struct sockaddr_in *sin, XIPAddr *addr)
{
	switch (addr->version)
	{
	case ipv4:
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = get_ip4(addr);
		break;
	case ipv6:
		sin->sin_family = AF_INET;
		assert(false);
		//sin->sin_addr.s_addr = get_ip6(addr);
		break;
	default:
		assert(false);
	}
}

void _net_ifc_configure_addresses(NetIfc *ni, TunIDAllocator* tunid)
{
	// can't be called until after tunid->open_tunnel() has had a chance
	// to allocate the user a tunid for the first time.

	ni->ifc4.netmask = decode_ipv4addr_assertsuccess("255.255.0.0");

	char ip_str[200];
	sprintf(ip_str, "10.%d.0.0", tunid->get_tunid());
	ni->ifc4.subnet = decode_ipv4addr_assertsuccess(ip_str);

	sprintf(ip_str, "10.%d.0.1", tunid->get_tunid());
	ni->ifc4.gateway = decode_ipv4addr_assertsuccess(ip_str);


	ni->ifc6.netmask = decode_ipv6addr_assertsuccess(
		"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00");
	ni->ifc6.subnet = decode_ipv6addr_assertsuccess(
		"fe80::");
	ni->ifc6.gateway = decode_ipv6addr_assertsuccess(
		"fe80::1");
}

void net_ifc_init_privileged(NetIfc *ni, Monitor *monitor, TunIDAllocator* tunid)
{
	int rc;

	ni->monitor = monitor;
	ni->use_proxy = USE_PROXY;

	ni->app_index = 2;
	int ioctlsocket = 0;

	if (USE_PROXY) {
		// well, we're not GOING to open a tunnel, so we'd better hope
		// the user has a tunid already for the purposes of ip address
		// assignment.
		_net_ifc_configure_addresses(ni, tunid);

		// Prepare a connection to the proxy
		ni->proxy_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (ni->proxy_sock < 0) {
			perror("proxy socket");
			assert(false);
		}

		bzero((char*)&ni->proxy_addr, sizeof(struct sockaddr_in));
		ni->proxy_addr.sin_family = AF_INET;
		ni->proxy_addr.sin_port = htons(HSN_DATA_PORT);

		if (inet_aton(PROXY_IP, &ni->proxy_addr.sin_addr) == 0) {
			perror("Proxy IP address translation failure");
			assert(false);
		}

		// Test the connection
		char buffer[] = "Hello world!";
		int rc = sendto(ni->proxy_sock, buffer, sizeof(buffer), 0, (sockaddr*)&ni->proxy_addr, sizeof(ni->proxy_addr));
		if (rc < 0) {
			perror("Sendto");
			assert(false);
		}
	} else {
		ni->tun_fd = tunid->open_tunnel();

		_net_ifc_configure_addresses(ni, tunid);

		ioctlsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

		fcntl(ni->tun_fd, F_SETFD, FD_CLOEXEC);

		struct ifreq ifr;
		tunid->setup_ifreq(&ifr);
		
		rc = ioctl(ioctlsocket, SIOCGIFFLAGS, &ifr);
		assert(rc==0);

		ifr.ifr_ifru.ifru_flags |= IFF_UP;
		rc = ioctl(ioctlsocket, SIOCSIFFLAGS, &ifr);
		assert(rc==0);

		ifr.ifr_mtu = 65535;
		rc = ioctl(ioctlsocket, SIOCSIFMTU, &ifr);
		assert(rc==0);

		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr;
		xip_to_sockaddr(sin, &ni->ifc4.gateway);
		rc = ioctl(ioctlsocket, SIOCSIFADDR, &ifr);
		assert(rc==0);

		sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_netmask;
		xip_to_sockaddr(sin, &ni->ifc4.netmask);
		rc = ioctl(ioctlsocket, SIOCSIFNETMASK, &ifr);
		assert(rc==0);

		sin = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_dstaddr;
		xip_to_sockaddr(sin, &ni->ifc4.subnet);
		rc = ioctl(ioctlsocket, SIOCSIFDSTADDR, &ifr);	// route this subnet here
		assert(rc==0);
	}

	hash_table_init(&ni->netconn_ht, monitor->get_mf(), net_connection_hash, net_connection_cmp);
	pthread_mutex_init(&ni->netconn_ht_mutex, NULL);

	rc = pthread_create(&ni->read_thread_obj, NULL, ni_read_thread, ni);
	assert(rc==0);

	if (!USE_PROXY) {
		rc = close(ioctlsocket);
		assert(rc==0);
	}

	_ni_init_forwarding_table(ni, monitor->get_mf());

	ni->packet_account = monitor->get_packet_account();
}

void *ni_read_thread(void *vni)
{
	NetIfc *ni = (NetIfc *) vni;

	uint32_t recv_size;
	int max_packet_size = 65536;
	char *buffer = (char*) malloc(max_packet_size);
	assert(buffer!=NULL);
	while (1)
	{
		if (USE_PROXY) {
			recv_size = recv(ni->proxy_sock, buffer, max_packet_size, 0); 
			printf("Read a packet of size %d from proxy\n", recv_size);
		} else {
			recv_size = read(ni->tun_fd, buffer, max_packet_size);
		}
		if (recv_size<=0)
		{
			perror("read");
			assert(0);
		}
		
		uint32_t iplen = 0; 
		char* data = NULL;
		if (USE_PROXY) {
			iplen = recv_size;
			data = buffer;
		} else {
			TunFrame *frame = (TunFrame*) buffer;
			assert(recv_size >= sizeof(TunFrame));
			iplen = recv_size - sizeof(TunFrame);
			data = frame->data;
		}

		IPInfo info;
		decode_ip_packet(&info, data, iplen);
		assert(info.packet_valid);
		assert(iplen == info.packet_length);

		PacketIfc *p = new SmallPacket(data, iplen, PacketIfc::FROM_TUNNEL, "read_from_tunnel");
		packet_account_create(ni->packet_account, p);

		//print_time("PACKET READ", p->len);

		send_packet(ni, &info, p);
	}
}

void _netifc_deliver_local(NetIfc *ni, PacketIfc *p, XIPAddr *dest)
{
	NetConnection key;
	key.host_ifconfig.local_interface = *dest;
	pthread_mutex_lock(&ni->netconn_ht_mutex);
	NetConnection *nconn = (NetConnection *) hash_table_lookup(&ni->netconn_ht, &key);
	pthread_mutex_unlock(&ni->netconn_ht_mutex);
	if (nconn!=NULL)
	{
		if (dest->version==ipv6)
		{
			char s_addr[100];
			format_ip(s_addr, sizeof(s_addr), dest);
			fprintf(stderr, "delivering ipv6 packet to %s; len %d\n", s_addr, p->len());
		}
		pq_insert(nconn->pq, p);
	}
	else
	{
		char s_addr[100];
		format_ip(s_addr, sizeof(s_addr), dest);
		fprintf(stderr, "dropping packet to %s; no matching dest.\n", s_addr);
//		forwarding_table_print(&ni->ft);
		packet_account_free(ni->packet_account, p);
		delete p;
	}
}

struct BroadcastClosure {
	PacketIfc *p;
	XIPVer ip_version;
	XIPAddr* dest;
	BroadcastClosure(PacketIfc *p, XIPVer ip_version, XIPAddr* dest)
		: p(p), ip_version(ip_version), dest(dest) {}
};

void ff_broadcast(void *user_obj, void *item)
{
	BroadcastClosure *bc = (BroadcastClosure *) user_obj;
	NetConnection *nconn = (NetConnection *) item;
//	char buff[100];
//	format_ip(buff, sizeof(buff), bc->dest);
//	fprintf(stderr, "Broadcasting packet to %s\n", buff);
	if (nconn->host_ifconfig.local_interface.version == bc->ip_version)
	{
		PacketIfc *copy = bc->p->copy("broadcast_copy");
		packet_account_create(nconn->ni->packet_account, copy);
//		format_ip(buff, sizeof(buff), &nconn->ai->conn4.host_ifconfig.local_interface);
//		fprintf(stderr, "\tBroadcasting packet to app IPv4: %s\n", buff);
//		format_ip(buff, sizeof(buff), &nconn->ai->conn6.host_ifconfig.local_interface);
//		fprintf(stderr, "\tBroadcasting packet to app IPv6: %s\n", buff);
#if 0
	{
		char s_addr[100];
		format_ip(s_addr, sizeof(s_addr), &nconn->host_ifconfig.local_interface);
		fprintf(stderr, "ff_broadcast delivered packet to %s\n", s_addr);
	}
#endif
		pq_insert(nconn->pq, copy);
	}
}

void _netifc_deliver_tunnel(NetIfc *ni, PacketIfc *p, XIPAddr *addr)
{
	if (p->len() >= 1<<20)
	{
		fprintf(stderr, "packet > 1MB; dropping on tunnel outbound link.\n");
	}
	else
	{
		if (USE_PROXY) { 
			int rc = sendto(ni->proxy_sock, p->data(), p->len(), 0, (sockaddr*)&ni->proxy_addr, sizeof(ni->proxy_addr));
			if (rc < 0) {
				perror("Sendto");
				assert(false);
			}
			printf("Sent a packet of size %d to the proxy\n", p->len());
		} else {
			int framelen = sizeof(TunFrame) + p->len();
			TunFrame *frame = (TunFrame *) alloca(framelen);
			frame->tpi.flags = 0;
			frame->tpi.proto = 8;	 // IP protocol (what's the symbolic name for that?)
			memcpy(frame->data, p->data(), p->len());

			int rc = write(ni->tun_fd, frame, framelen);
			if (rc!=framelen) { perror("write"); }
			assert(rc==framelen);
		}
	}

	packet_account_free(ni->packet_account, p);
	delete p;
}

void _ni_init_forwarding_table(NetIfc *ni, MallocFactory *mf)
{
	forwarding_table_init(&ni->ft, mf);

	// default route
	XIPAddr zero4 = decode_ipv4addr_assertsuccess("0.0.0.0");
	XIPAddr zero6 = decode_ipv6addr_assertsuccess("::");
	XIPAddr one4 = decode_ipv4addr_assertsuccess("255.255.255.255");
	XIPAddr one6 = decode_ipv6addr_assertsuccess("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
	XIPAddr bcast4 = xip_broadcast(&ni->ifc4.gateway, &ni->ifc4.netmask);
	XIPAddr bcast6 = xip_broadcast(&ni->ifc6.gateway, &ni->ifc6.netmask);

	forwarding_table_add(&ni->ft, &zero4, &zero4, _netifc_deliver_tunnel, "tunnel");
	forwarding_table_add(&ni->ft, &zero6, &zero6, _netifc_deliver_tunnel, "tunnel");

	// link broadcast
	forwarding_table_add(&ni->ft, &one4, &one4, _netifc_deliver_broadcast, "broadcast");
	forwarding_table_add(&ni->ft, &one6, &one6, _netifc_deliver_broadcast, "broadcast");
		// TODO NB there isn't actually an ipv6 broadcast address like this.
		// Nor are we using it. Can probably just remove this entry.

	// link-wide route (to apps)
	forwarding_table_add(&ni->ft, &ni->ifc4.subnet, &ni->ifc4.netmask, _netifc_deliver_local, "local");
	forwarding_table_add(&ni->ft, &ni->ifc6.subnet, &ni->ifc6.netmask, _netifc_deliver_local, "local");

	// link-specific broacast addr (needs to match before link-wide route)
	// Don't re-transmit packet out to tunnel
	forwarding_table_add(&ni->ft, &bcast4, &one4, _netifc_deliver_broadcast, "link-broadcast");
	forwarding_table_add(&ni->ft, &bcast6, &one6, _netifc_deliver_broadcast, "link-broadcast");

	// gateway, specifically.
	// (needed since netmask matches on link otherwise hit deliver_local)
	forwarding_table_add(&ni->ft, &ni->ifc4.gateway, &one4, _netifc_deliver_tunnel, "tunnel");
	forwarding_table_add(&ni->ft, &ni->ifc6.gateway, &one6, _netifc_deliver_tunnel, "tunnel");

	forwarding_table_print(&ni->ft);
}

void send_packet(NetIfc *ni, XPBPoolElt *xpbe)
{
	char *data = (char*) xpbe->xpb->un.znb._data;
	uint32_t len = xpbe->xpb->capacity;

	IPInfo info;
	decode_ip_packet(&info, data, len);
	if (!info.packet_valid)
	{
		fprintf(stderr, "packet invalid because %s\n", info.failure);
		assert(info.packet_valid);
	}
	assert(info.packet_length <= len);

	if (info.packet_length > 1<<20)
	{
		XNB_NOTE("send_packet is big (>1MiB)\n");
	}

	PacketIfc *p;
	if (info.packet_length > 1<<16)
	{
		p = new XPBPacket(xpbe, PacketIfc::FROM_APP);
	}
	else
	{
		p = new SmallPacket(data, info.packet_length, PacketIfc::FROM_APP, "unset");
	}

	send_packet(ni, &info, p);
}

#if 0 // dead code?
void send_packet(NetIfc *ni, char *buf, uint32_t capacity)
{
	IPInfo info;
	decode_ip_packet(&info, buf, capacity);
	if (!info.packet_valid)
	{
		fprintf(stderr, "packet invalid because %s\n", info.failure);
		assert(info.packet_valid);
	}
	assert(info.packet_length <= capacity);

	PacketIfc *p = new SmallPacket(buf, info.packet_length, "unset");

	send_packet(ni, &info, p);
}
#endif

void send_packet(NetIfc *ni, IPInfo *info, PacketIfc *p)
{
#if 0
	{
		char srcbuf[100], dstbuf[100];
		format_ip(srcbuf, sizeof(srcbuf), &info->source);
		format_ip(dstbuf, sizeof(dstbuf), &info->dest);
		fprintf(stderr, "packet from %s to %s delivering to %s, size %d\n",
			srcbuf, dstbuf, fe->deliver_dbg_name, info->packet_length);
	}
#endif

	ForwardingEntry *fe = forwarding_table_lookup(&ni->ft, &info->dest);
	p->dbg_set_purpose(fe->deliver_dbg_name);
	packet_account_create(ni->packet_account, p);
	(fe->deliver_func)(ni, p, &info->dest);
}

void _netifc_deliver_broadcast(NetIfc *ni, PacketIfc *p, XIPAddr *dest)
{
	// subnet broadcast
	pthread_mutex_lock(&ni->netconn_ht_mutex);
	BroadcastClosure bc(p, dest->version, dest);
	hash_table_visit_every(&ni->netconn_ht, ff_broadcast, &bc);
	pthread_mutex_unlock(&ni->netconn_ht_mutex);

	if (p->get_origin_type() == PacketIfc::FROM_TUNNEL)
	{
		// well, don't send it back there!
		packet_account_free(ni->packet_account, p);
		delete p;
	}
	else
	{
		_netifc_deliver_tunnel(ni, p, dest);
		// ni_deliver_tunnel accepts ownership of packet object
	}
}

#define QUEUE_MAX_BYTES		(256*(1<<20))
#define QUEUE_MIN_PACKETS	(3)

void app_interface_init(AppInterface *ai, NetIfc *ni, EventSynchronizer *event_synchronizer)
{
	pq_init(&ai->pq, ni->packet_account, QUEUE_MAX_BYTES, QUEUE_MIN_PACKETS, event_synchronizer);

	int app_index = _net_connection_allocate_index(ni);
	_net_connection_init(&ai->conn4, ni, &ni->ifc4, app_index, &ai->pq, ai);
	_net_connection_init(&ai->conn6, ni, &ni->ifc6, app_index, &ai->pq, ai);
}

void app_interface_free(AppInterface *ai, NetIfc *ni)
{
	_net_connection_free(&ai->conn4, ni);
	_net_connection_free(&ai->conn6, ni);
	//pq_free(&ai->pq);
}

void _net_connection_init(NetConnection *nconn, NetIfc *ni, XIP_ifc_config *ifconfig, int app_index, PacketQueue *pq, AppInterface *ai)
{
	nconn->ni = ni;
	nconn->pq = pq;
	nconn->ai = ai;

	nconn->host_ifconfig.netmask = ifconfig->netmask;
	nconn->host_ifconfig.gateway = ifconfig->gateway;

	// construct app ip addr by taking gateway address and replacing
	// last octet
	nconn->host_ifconfig.local_interface = ifconfig->gateway;
	switch (nconn->host_ifconfig.local_interface.version)
	{
	case ipv4:
	{
		uint8_t *ip_octets = (uint8_t*)
			&nconn->host_ifconfig.local_interface.xip_addr_un.xv4_addr;
		ip_octets[3] = app_index;
		break;
	}
	case ipv6:
	{
		// construct app ip addr by taking gateway address and replacing
		// last octet
		uint8_t *ip_octets =
			nconn->host_ifconfig.local_interface.xip_addr_un.xv6_addr.addr;
		ip_octets[15] = app_index;
		break;
	}
	default:
		lite_assert(false);	// unexpected ip version
	}

	pthread_mutex_lock(&ni->netconn_ht_mutex);
	hash_table_insert(&ni->netconn_ht, nconn);
	pthread_mutex_unlock(&ni->netconn_ht_mutex);
}

void _net_connection_free(NetConnection *nconn, NetIfc *ni)
{
	pthread_mutex_lock(&ni->netconn_ht_mutex);
	hash_table_remove(&ni->netconn_ht, nconn);
	pthread_mutex_unlock(&ni->netconn_ht_mutex);
}

#if 0 // dead code
void app_interface_send_timer_tick(AppInterface *ai)
{
	// only send one of these silly ticks to each app; use its ipv4 addr
	NetConnection *nconn = &ai->conn4;

	Packet *p = packet_alloc(
		udp_packet_length(nconn->host_ifconfig.local_interface.version, 0),
		ai->conn4.ni->packet_account,
		"send_timer_tick");

	UDPEndpoint src_ep;
	src_ep.ipaddr = nconn->host_ifconfig.gateway;
	src_ep.port = htons(UDP_PORT_FAKE_TIMER);

	UDPEndpoint dst_ep;
	dst_ep.ipaddr = nconn->host_ifconfig.local_interface;
	dst_ep.port = htons(UDP_PORT_FAKE_TIMER);

	create_udp_packet_buf((uint8_t*) packet_data(p), p->len, src_ep, dst_ep, 0);

//		print_time("TICK", p->len);
	pq_insert(nconn->pq, p);
}
#endif

uint32_t net_connection_hash(const void *a)
{
	NetConnection *nca = (NetConnection *) a;
	return hash_xip(&nca->host_ifconfig.local_interface);
}

int net_connection_cmp(const void *a, const void *b)
{
	NetConnection *nca = (NetConnection *) a;
	NetConnection *ncb = (NetConnection *) b;
	return cmp_xip(
		&nca->host_ifconfig.local_interface,
		&ncb->host_ifconfig.local_interface);
}

int _net_connection_allocate_index(NetIfc *ni)
{
	assert(ni->app_index < 255);	// uh oh, ran out of (arbitrarily hard-coded) netmasked space
	int result = ni->app_index;
	ni->app_index+=1;
	return result;
}
