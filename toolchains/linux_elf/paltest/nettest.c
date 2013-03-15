#include "paltest.h"
#include "simple_network.h"

#define IP_IPV4 4
#define IP_ICMP 1

#define ICMP_ECHO 8 
#define ICMP_ECHOREPLY 0  
#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header)  

#if 0
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned long DWORD;

#pragma pack(1)

typedef struct
{
	unsigned int version:4;			// Version of IP
	unsigned int h_len:4;			// length of the header
	unsigned int codepoint:6;		// differentiated services codepoint
	unsigned int unused:2;			// unused
	unsigned int total_len:16;		// total length of the packet
	unsigned int ident:16;			// unique identifier
	unsigned int flags:4;			// flags
	unsigned int frag:12;			// fragmentation information
	unsigned int ttl:8;				// time to live
	unsigned int proto:8;			// protocol (TCP, UDP, etc.)
	unsigned int checksum:16;		// IP checksum
	unsigned int sourceIP;			// source address
	unsigned int destIP;			// destination address
} IpHeader;

typedef struct {
	uint32_t length;
	IpHeader header;
} XaxIPBuffer;
static __inline void *XIPB_PAYLOAD(XaxIPBuffer *xipb) { return (void*) (xipb+1); }

typedef struct
{
	BYTE i_type;
	BYTE i_code;
	USHORT i_cksum;
	USHORT i_id;
	USHORT i_seq;
	DWORD responder_id;
	DWORD timestamp;
} IcmpPacket;

XaxNetBuffer *
alloc_and_recv_net_buffer()
{
	XaxNetBuffer *xnb = (g_zdt->xax_alloc_net_buffer)(0);
	int recv_status = (g_zdt->xax_recv_net_buffer)(xnb);
	if (recv_status != XAX_ERROR_BUFFER_TOO_SMALL)
	{
		(g_debuglog)("stderr", "xax_recv_net_buffer() failed to return XAX_ERROR_BUFFER_TOO_SMALL on zero-length buffer.\n");
		(g_zdt->xax_exit)();
	}
	uint32_t length = xnb->length;
	(g_zdt->xax_free_net_buffer)(xnb);
	xnb = 0;

	xnb = (g_zdt->xax_alloc_net_buffer)(length);
	recv_status = (g_zdt->xax_recv_net_buffer)(xnb);
	if (recv_status != XAX_SUCCESS)
	{
		(g_debuglog)("stderr", "xax_recv_net_buffer() failed to return XAX_SUCCESS.\n");
		(g_zdt->xax_exit)();
	}
	return xnb;
}

XaxIPBuffer *
allocate_ip_buffer(
	size_t payload_length)
{
	size_t packet_length = sizeof(IpHeader) + payload_length;
	XaxIPBuffer *buffer = (XaxIPBuffer *)((g_zdt->xax_alloc_net_buffer)(packet_length));
	if (buffer == 0)
	{
		(g_debuglog)("stderr", "xax_alloc_net_buffer() returned NULL pointer.\n");
		(g_zdt->xax_exit)();
	}
	return buffer;
}
#endif

void
send_one_big_packet(int label)
{
	const int payload_len = 1400;

	UDPEndpoint src = const_udp_endpoint_ip4(10,7,7,7, 7777);
	UDPEndpoint dst = const_udp_endpoint_ip4(10,8,8,8, 8888);
	uint32_t *payload;
	ZoogNetBuffer *znb =
		create_udp_packet_xnb(g_zdt, src, dst, payload_len, (void**) &payload);
	
	uint32_t v;
	for (v=0; v<(payload_len/sizeof(v)); v++)
	{
		payload[v] = (label<<24) | (v+10);
	}

	(g_zdt->zoog_send_net_buffer)(znb, true);
}

void
nettest()
{
	XIPifconfig ifconfigs[2];
	int count = 2;
	(g_zdt->zoog_get_ifconfig)(ifconfigs, &count);

	int i;
	for (i=0; i<100; i++)
	{
		send_one_big_packet(i);
	}
}
