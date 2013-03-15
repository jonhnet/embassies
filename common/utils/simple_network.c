#include "simple_network.h"
#include "xax_endian.h"
#include "LiteLib.h"
#include "xax_network_utils.h"

uint16_t ip_checksum(IP4Header *ipp)
{
	uint16_t *word_ptr = (uint16_t*) ipp;
	uint32_t acc = 0;
	int i=0;
	int num_words = 2*(ipp->version_and_ihl&0x0f);
	for (i=0; i<num_words; i++, word_ptr++)
	{
		acc += *word_ptr;
	}
	acc = (acc>>16) + (acc & 0xffff);
	acc = (acc>>16) + (acc & 0xffff);	// possibly one more bit
	acc = (~acc) & 0xffff;	// invert
	return (uint16_t) acc;
}

static bool use_jumbogram(uint32_t ip_payload_size)
{
	return ip_payload_size >= 65536;
}

static uint32_t ip_header_size(XIPVer version, uint32_t ip_payload_size)
{
	switch (version)
	{
		case ipv4:
			return sizeof(IP4Header);
		case ipv6:
			if (use_jumbogram(ip_payload_size))
			{
				return sizeof(IP6Header)
					+sizeof(XIP6ExtensionHeader)
					+sizeof(XIP6Option)
					+sizeof(XIP6OptionJumbogram);
			}
			else
			{
				return sizeof(IP6Header);
			}
		default:
			lite_assert(false);
			return 0;
	}
}

uint32_t udp_packet_length(XIPVer version, uint32_t payload_size)
{
	uint32_t ip_payload_size = sizeof(UDPPacket)+payload_size;
	return ip_header_size(version, ip_payload_size) + ip_payload_size;
}

static void *create_udp_packet_buf_v4(
	uint8_t *local_buffer, uint32_t buf_capacity, UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size)
{
	IP4Header *ipp = (IP4Header*) local_buffer;
	ipp->version_and_ihl = 0x45;
	ipp->type_of_service = 0;
	ipp->total_length = z_htong(udp_packet_length(ipv4, payload_size),
		sizeof(ipp->total_length));
	ipp->identification = 0;
	ipp->flags_and_fragment_offset = 0;
	ipp->time_to_live = 0xff;
	ipp->protocol = PROTOCOL_UDP;
	ipp->header_checksum = 0;
	ipp->source_ip = get_ip4(&source_ep.ipaddr);
	ipp->dest_ip = get_ip4(&dest_ep.ipaddr);
	ipp->header_checksum = ip_checksum(ipp);

	UDPPacket *udp = (UDPPacket*) &ipp[1];
	udp->source_port = source_ep.port;
	udp->dest_port = dest_ep.port;
	udp->length = xax_htons(sizeof(UDPPacket)+payload_size);
	udp->checksum = 0;

	void *payload = (uint16_t*) &udp[1];
	return payload;
}

static void *create_udp_packet_buf_v6(
	uint8_t *local_buffer, uint32_t buf_capacity, UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size)
{
	uint32_t ip_payload_size = sizeof(UDPPacket) + payload_size;
	bool jumbo = use_jumbogram(ip_payload_size);

	IP6Header *ip6 = (IP6Header *) local_buffer;
	ip6->vers_class_flow = z_htong(6<<28, sizeof(ip6->vers_class_flow));
	ip6->payload_len = z_htong(
		jumbo ? 0 : ip_payload_size,
		sizeof(ip6->payload_len));
	ip6->next_header_proto = z_htong(
		jumbo ? IP_PROTO_OPTION_HEADER : PROTOCOL_UDP,
		sizeof(ip6->next_header_proto));
	ip6->hop_limit = z_htong(253, sizeof(ip6->hop_limit));
	ip6->source_ip6 = get_ip6(&source_ep.ipaddr);
	ip6->dest_ip6 = get_ip6(&dest_ep.ipaddr);

	void *next = (void*) &ip6[1];
	if (jumbo)
	{
		// these assignments are, save the last, single octets, so no HTON
		XIP6ExtensionHeader *opt_hdr = (XIP6ExtensionHeader *) next;
		opt_hdr->next_header_proto = PROTOCOL_UDP;
		lite_assert(sizeof(XIP6ExtensionHeader)+sizeof(XIP6Option)+sizeof(XIP6OptionJumbogram) == 8);
		opt_hdr->header_len = 0;
			// length of extn header in units of 8 octets,
			// minus 1. So this 0 => extn header is 8 bytes long.
		XIP6Option *opt = (XIP6Option *) &opt_hdr[1];
		opt->option_type = IP6_OPTION_TYPE_JUMBOGRAM;
		opt->option_length = sizeof(XIP6OptionJumbogram);
		XIP6OptionJumbogram *jumbo_opt = (XIP6OptionJumbogram *) &opt[1];
		jumbo_opt->jumbo_length = z_htong(ip_payload_size, sizeof(jumbo_opt->jumbo_length));
		next = ((void*) &opt[1]) + opt->option_length;
	}
	int actual_header_size = next - (void*) local_buffer;
	lite_assert(payload_size <= buf_capacity - actual_header_size);

	UDPPacket *udp = (UDPPacket*) next;
	udp->source_port = source_ep.port;
	udp->dest_port = dest_ep.port;
	udp->length = jumbo ? 0 : xax_htons(sizeof(UDPPacket)+payload_size);
	udp->checksum = 0;

	next = (void*) &udp[1];

	return next;
}

void *create_udp_packet_buf(
	uint8_t *local_buffer, uint32_t buf_capacity, UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size)
{
	lite_assert(buf_capacity
		>= udp_packet_length(dest_ep.ipaddr.version, payload_size));

	switch (dest_ep.ipaddr.version)
	{
		case ipv4:
			return create_udp_packet_buf_v4(local_buffer, buf_capacity, source_ep, dest_ep, payload_size);
		case ipv6:
			return create_udp_packet_buf_v6(local_buffer, buf_capacity, source_ep, dest_ep, payload_size);
		default:
			lite_assert(false);
			return NULL;
	}
}

ZoogNetBuffer *create_udp_packet_xnb(
	ZoogDispatchTable_v1 *zdt,
	UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size,
	void **out_payload)
{
	uint32_t total_length =
		udp_packet_length(dest_ep.ipaddr.version, payload_size);
	ZoogNetBuffer *znb = (zdt->zoog_alloc_net_buffer)(total_length);
	*out_payload = create_udp_packet_buf(
		(uint8_t *) &znb[1], znb->capacity, source_ep, dest_ep, payload_size);
	return znb;
}

UDPEndpoint const_udp_endpoint_ip4(
	uint8_t oct0, uint8_t oct1, uint8_t oct2, uint8_t oct3, uint16_t port)
{
	UDPEndpoint ep;
	ep.ipaddr.version = ipv4;
	ep.ipaddr.xip_addr_un.xv4_addr = (oct3<<24) | (oct2<<16) | (oct1<<8) | oct0;
	ep.port = xax_htons(port);
	return ep;
}

