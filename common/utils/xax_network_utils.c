// NB right now this file is only useful for posix-environment builds;
// it depends on inet_pton().

/*
#include <arpa/inet.h>
#include <assert.h>
*/

#include "LiteLib.h"
#include "xax_network_utils.h"
#include "cheesy_snprintf.h"
#include "hash_table.h"

#define Z_OFFSETOF(s, f)	((uint32_t)(((uint8_t*)&(s->f)) - ((uint8_t*)(s))))

bool decode_ipv4addr(const char *str, XIP4Addr *out_result)
{
	XIP4Addr result = 0;
	uint32_t octet = 0;
	int num_octets = 0;

	int si;
	for (si=0; ; si++)
	{
		if (str[si]=='.' || str[si]=='\0')
		{
			if (octet>255)
			{
				return false;
			}
			if (num_octets == 4)
			{
				return false;
			}
			// stupid little-endian
			// TODO LITTLE-ENDIAN ASSUMPTION
			result = (result >> 8) | (octet<<24);
			num_octets += 1;
			octet = 0;
			if (str[si]=='\0')
			{
				break;
			}
		}
		else if (str[si]>='0' && str[si]<='9')
		{
			octet = octet*10 + (str[si]-'0');
		}
		else
		{
			return false;
		}
	}
	if (num_octets != 4)
	{
		return false;
	}

	*out_result = result;
	return true;
}

XIPAddr decode_ipv4addr_assertsuccess(const char *str)
{
	XIP4Addr result4;
	lite_memset(&result4, 0, sizeof(result4));	// satisfy compiler that we initialized
		// weird. Why does lite_memset satisfy it, when the call to decode doesn't?
	bool rc = decode_ipv4addr(str, &result4);
	lite_assert(rc);
	XIPAddr result = ip4_to_xip(&result4);
	return result;
}

typedef struct {
	uint16_t clumps[8];
	int idx;
} ClumpList;

void cl_init(ClumpList *cl)
{
	cl->idx = 0;
}

int cl_len(ClumpList *cl)
{
	return cl->idx;
}

void cl_push(ClumpList *cl, uint16_t clump)
{
	lite_assert(cl->idx < (sizeof(cl->clumps)/sizeof(cl->clumps[0])));
	cl->clumps[cl->idx] = clump;
	cl->idx++;
}

uint16_t cl_read(ClumpList *cl, int idx)
{
	lite_assert(idx>=0 && idx<cl->idx);
	return cl->clumps[idx];
}

bool is_all_hex(const char *start, const char *end)
{
	const char *p;
	for (p=start; p<end; p++)
	{
		if (   (p[0]>='0' && p[0]<='9')
			|| (p[0]>='a' && p[0]<='f')
			|| (p[0]>='A' && p[0]<='F'))
		{
			continue;
		}
		return false;
	}
	return true;
}

// returns NULL on success, or an error string
const char *decode_ipv6addr(const char *str, XIPAddr *out_addr)
{
	ClumpList prefix, suffix;
	cl_init(&prefix);
	cl_init(&suffix);
	ClumpList *curlist = &prefix;

	const char *p;
	const char *end_term = NULL;

	if (str[0]==':')
	{
		if (str[1]!=':')
		{
			return "lonely colon prefix";
		}
		p = &str[2];
		curlist = &suffix;
	}
	else
	{
		p = &str[0];
	}

	bool done=false;
	for (; !done; p=end_term+1)
	{
		end_term = lite_index(p, ':');
		if (end_term!=NULL && end_term==p)
		{
			// this is an empty group
			if (curlist == &suffix)
			{
				return "two compressed regions";
			}
			curlist = &suffix;
			continue;
		}
		else if (end_term==NULL)
		{
			end_term = &p[lite_strlen(p)];
			if (end_term==p)
			{
				// p points to an empty string, after a colon. drat.
				if (curlist==&suffix && cl_len(&suffix)==0)
				{
					// that's okay -- we just finished consuming
					// a compressed region, and it happens to be
					// the tail. We're done.
					break;
				}
				else
				{
					return "single trailing colon";
				}
			}
			// This is the last region; end_term+1 is undefined. Note that,
			// but still parse last clump.
			done = true;
		}


		if (cl_len(&prefix)+cl_len(&suffix)==8)
		{
			return "too many clumps";
		}
		if (!is_all_hex(p, end_term))
		{
			return "non-hex term";
		}
		if (end_term - p > 4)
		{
			return "term more than four nybbles";
		}
		cl_push(curlist, str2hex(p));
	}

	if (curlist==&prefix && cl_len(&prefix)!=8)
	{
		return "not enough terms";
	}

	out_addr->version = ipv6;
	int out_idx = 0;
	int in_idx;
	for (in_idx=0; in_idx<cl_len(&prefix); in_idx++)
	{
		uint16_t v = cl_read(&prefix, in_idx);
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = (v>>8) & 0xff;
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = (v>>0) & 0xff;
	}
	for (; in_idx < 8 - cl_len(&suffix); in_idx++)
	{
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = 0;
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = 0;
	}
	for (in_idx = 0; in_idx < cl_len(&suffix); in_idx++)
	{
		uint16_t v = cl_read(&suffix, in_idx);
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = (v>>8) & 0xff;
		out_addr->xip_addr_un.xv6_addr.addr[out_idx++] = (v>>0) & 0xff;
	}
	lite_assert(out_idx==16);
	return NULL;
}

XIPAddr decode_ipv6addr_assertsuccess(const char *str)
{
	XIPAddr result;
	const char *failure = decode_ipv6addr(str, &result);
	lite_assert(failure==NULL);
	return result;
}

XIP4Addr literal_ipv4addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return
		  (((uint32_t)a)<< 0)
		| (((uint32_t)b)<< 8)
		| (((uint32_t)c)<<16)
		| (((uint32_t)d)<<24);
}

XIPAddr get_ip_subnet_broadcast_address(XIPVer version)
{
	switch (version)
	{
	case ipv4:
	{
		XIP4Addr a = literal_ipv4addr(255,255,255,255);
		XIPAddr xp = ip4_to_xip(&a);
		return xp;
	}
	case ipv6:
	{
		XIPAddr xp6 = decode_ipv6addr_assertsuccess(
			"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
			// TODO not really an IPv6 address; I made it up.
		return xp6;
	}
	default:
		lite_assert(false);
		XIPAddr dummy;
		lite_memset(&dummy, 0, sizeof(XIPAddr));
		return dummy;
	}
}

bool is_ip_subnet_broadcast_address(XIPAddr *addr)
{
	switch (addr->version)
	{
	case ipv4:
		return lite_memequal((char*) &addr->xip_addr_un.xv4_addr, (char)255, sizeof(addr->xip_addr_un.xv4_addr));
	case ipv6:
		return lite_memequal((char*) &addr->xip_addr_un.xv6_addr, (char)0xff, sizeof(addr->xip_addr_un.xv6_addr));
	default:
		lite_assert(false);
		return false;
	}
}

bool _6len_scan_options_for_length(
	IPInfo *info,
	uint8_t *packet, uint32_t capacity,
	uint32_t header_offset, uint32_t option_offset, uint32_t ext_hdr_len)
{
	// Caller already checked that header length fits in capacity;
	// now we're scanning its options
	while (true)
	{
		if (option_offset + sizeof(XIP6Option) > capacity)
		{
			info->failure = "packet too short to contain this option header";
			return false;
		}
		if (option_offset + sizeof(XIP6Option) - header_offset
			> ext_hdr_len)
		{
			info->failure = "header too short to contain this option header";
			return false;
		}
		XIP6Option *ip6option = (XIP6Option *) (packet + option_offset);
		if (option_offset + sizeof(XIP6Option) + ip6option->option_length
			> capacity)
		{
			info->failure = "packet too short to contain this option";
			return false;
		}
		if (option_offset + ip6option->option_length - header_offset
			> ext_hdr_len)
		{
			info->failure="header too short to contain this option";
			return false;
		}
		if (ip6option->option_type==IP6_OPTION_TYPE_JUMBOGRAM)
		{
			if (ip6option->option_length != sizeof(XIP6OptionJumbogram))
			{
				info->failure="invalid jumbogram option length";
				return false;
			}
			XIP6OptionJumbogram *ip6jumbo = (XIP6OptionJumbogram *)
				(packet + option_offset + sizeof(XIP6Option));
			if (info->found_jumbo_length)
			{
				info->failure = "found two jumbo lengths";
				return false;
			}
			info->found_jumbo_length = true;
			info->payload_length = Z_NTOHG(ip6jumbo->jumbo_length);
		}

		// nope, not that option. Next.
		if (ip6option->option_length==0)
		{
			// nice try, infinite-loop-man.
			info->failure = "zero-length option";
			return false;
		}
		option_offset = option_offset + sizeof(XIP6Option) + ip6option->option_length;

		if (option_offset - header_offset == ext_hdr_len)
		{
			break;	// end of options in this header.
		}
		if (option_offset - header_offset > ext_hdr_len)
		{
			lite_assert(false); // should have discovered this length mismatch already
			return false;
		}
	}
	// completed scanning this option set
	return true;
}

void _6len_scan_ext_headers_for_length(
	IPInfo *info,
	uint8_t *packet, uint32_t capacity,
	uint32_t header_type_offset, uint32_t header_offset)
{
	while (true)
	{
		if (header_type_offset + sizeof(uint8_t) > capacity)
		{
			info->failure = "bad header chain";
			return;
		}
		uint8_t this_header_type = ((uint8_t*) packet)[header_type_offset];
			// I'd put a HTON here, but even the cast says it's a byte.
		if (this_header_type != IP_PROTO_OPTION_HEADER)
		{
			if (!info->found_jumbo_length)
			{
				info->failure = "Never found length field";
				return;
			}
			info->layer_4_protocol = this_header_type;
			info->payload_offset = header_offset;
			info->packet_valid = true;
			return;
		}

		XIP6ExtensionHeader *ext_hdr =
			(XIP6ExtensionHeader *) (packet + header_offset);
		uint32_t ext_hdr_len_encoded = Z_NTOHG(ext_hdr->header_len);
		uint32_t ext_hdr_len = (ext_hdr_len_encoded+1)*8;
		if (header_offset + ext_hdr_len > capacity)
		{
			info->failure = "packet too short to contain claimed option header length";
			return;
		}

		bool found_in_options = _6len_scan_options_for_length(
			info, packet, capacity,
			header_offset,
			header_offset + sizeof(XIP6ExtensionHeader),
			ext_hdr_len);
		if (!found_in_options)
		{
			return;
		}

		// nope, not this header. Next.
		if (ext_hdr_len==0)
		{
			// nice try, infinite-loop-man.
			info->failure = "zero-length option header";
			return;
		}
		header_type_offset = header_offset + Z_OFFSETOF(ext_hdr, next_header_proto);
		header_offset = header_offset + ext_hdr_len;
	}
	// No headers with jumbo option in them. Sorry.
	info->failure = "jumbo option missing";
	return;
}

// confused by extensions/option headers and interpretation of goofy
// length fields? Enjoy:
// http://www.zytrax.com/tech/protocols/ipv6.html#extension
void decode_ip6_packet(IPInfo *info, void *packet, uint32_t capacity)
{
	if (capacity < sizeof(IP6Header))
	{
		info->failure = "packet smaller than basic IPv6 header";
		return;
	}

	IP6Header *ip6header = (IP6Header *) packet;

	info->source.version = ipv6;
	info->source.xip_addr_un.xv6_addr = ip6header->source_ip6;
	info->dest.version = ipv6;
	info->dest.xip_addr_un.xv6_addr = ip6header->dest_ip6;
	info->more_fragments = false;	// ipv6 fragments not supported
	info->fragment_offset = 0;

	uint32_t basic_len = Z_NTOHG(ip6header->payload_len);
	if (basic_len > 0)
	{
		// not jumbo.
		info->payload_length = basic_len;
		info->packet_valid = true;
		info->layer_4_protocol = ip6header->next_header_proto;
		info->payload_offset = sizeof(*ip6header);
		// NB this calculation of payload_offset will fail if other
		// extension headers are present; we should still scan_ext_headers...
		// but first set
		//	info->found_jumbo_length = true
		// to avoid failing the "Never found length field" test.
		// (Well, that would be hard to read, but something logically equiv.)
	}
	else
	{
		_6len_scan_ext_headers_for_length(info, (uint8_t*)packet, capacity,
			Z_OFFSETOF(ip6header, next_header_proto),
			sizeof(*ip6header));
	}
	info->packet_length = info->payload_offset + info->payload_length;
}

void decode_ip4_packet(IPInfo *info, void *packet, uint32_t capacity)
{
	if (capacity < sizeof(IP4Header))
	{
		info->failure = "packet smaller than basic IPv4 header";
		return;
	}

	IP4Header *ip4header = (IP4Header *) packet;
	info->packet_length = Z_NTOHG(ip4header->total_length);
	info->payload_length = info->packet_length - sizeof(IP4Header);
	info->layer_4_protocol = ip4header->protocol;
	info->payload_offset = sizeof(*ip4header);

	uint16_t frag_field = Z_NTOHG(ip4header->flags_and_fragment_offset);
	info->more_fragments = (frag_field & MORE_FRAGMENTS_BIT) != 0;
	info->fragment_offset = ((frag_field & FRAGMENT_OFFSET_MASK) << FRAGMENT_GRANULARITY_BITS);
	info->identification = Z_NTOHG(ip4header->identification);

	info->source.version = ipv4;
	info->source.xip_addr_un.xv4_addr = ip4header->source_ip;
	info->dest.version = ipv4;
	info->dest.xip_addr_un.xv4_addr = ip4header->dest_ip;
	info->packet_valid = true;
	return;
}

XIPVer decode_ip_version(void *packet)
{
	uint8_t vers_field = ((uint8_t*)packet)[0] >> 4;
	switch (vers_field)
	{
	case 4:
		return ipv4;
	case 6:
		return ipv6;
	default:
		return ipv_err;
	}
}

void decode_ip_dest(void *packet, XIPAddr *out_addr)
{
	switch (decode_ip_version(packet))
	{
		case ipv6:
		{
		}
		default:
		{
			lite_assert(false);
		}
	}
}

void decode_ip_packet(IPInfo *info, void *packet, uint32_t capacity)
{
	lite_memset(info, 0, sizeof(*info));

	if (capacity < 1)
	{
		info->failure = "zero-length packet";
		goto done;
	}

	switch (decode_ip_version(packet))
	{
		case ipv4:
			decode_ip4_packet(info, packet, capacity);
			break;
		case ipv6:
			decode_ip6_packet(info, packet, capacity);
			break;
		default:
			info->failure = "invalid ip version field";
			break;
	}

done:
	if (info->packet_valid)
	{
		lite_assert(info->failure==NULL);
		lite_assert(info->payload_length > 0);
		lite_assert(info->layer_4_protocol != 0);
			// this could be a valid packet value; making this line a DOS attack
		lite_assert(info->payload_offset > 0);
	}
	else
	{
		// failure path didn't document why the packet failed.
		lite_assert(info->failure!=NULL);
	}
}

XIP4Addr get_ip4(XIPAddr *addr)
{
	lite_assert(addr->version == ipv4);
	return addr->xip_addr_un.xv4_addr;
}

XIP6Addr get_ip6(XIPAddr *addr)
{
	lite_assert(addr->version == ipv6);
	return addr->xip_addr_un.xv6_addr;
}

XIPAddr ip4_to_xip(XIP4Addr *addr)
{
	XIPAddr xip;
	xip.version = ipv4;
	xip.xip_addr_un.xv4_addr = *addr;
	return xip;
}

XIPAddr literal_ipv4xip(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	XIP4Addr addr = literal_ipv4addr(a, b, c, d);
	return ip4_to_xip(&addr);
}

uint32_t hash_xip(XIPAddr *addr)
{
	if (addr->version==ipv4)
	{
		return addr->xip_addr_un.xv4_addr;
	}
	else
	{
		return hash_buf(
			&addr->xip_addr_un.xv6_addr,
			sizeof(addr->xip_addr_un.xv6_addr));
	}
}

int cmp_xip(XIPAddr *a, XIPAddr *b)
{
	int result = a->version - b->version;
	if (result!=0) { return result; }

	if (a->version==ipv4)
	{
		return a->xip_addr_un.xv4_addr - b->xip_addr_un.xv4_addr;
	}
	else
	{
		return lite_memcmp(
			&a->xip_addr_un.xv6_addr,
			&b->xip_addr_un.xv6_addr,
			sizeof(a->xip_addr_un.xv6_addr));
	}
}

UDPEndpoint *make_UDPEndpoint(MallocFactory *mf, XIPAddr *addr, uint16_t port)
{
	UDPEndpoint *ep = (UDPEndpoint *)mf_malloc(mf, sizeof(UDPEndpoint));
	ep->ipaddr = *addr;
	ep->port = z_htong(port, sizeof(ep->port));
	return ep;
}

XIPAddr xip_all_ones(XIPVer ver)
{
	// ugly helper used for netmasks and broadcast addrs

	switch (ver)
	{
	case ipv4:
		return decode_ipv4addr_assertsuccess("255.255.255.255");
	case ipv6:
		return decode_ipv6addr_assertsuccess("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
		// TODO NB there isn't actually an ipv6 broadcast address like this.
		// Nor are we using it. Perhaps just remove this entry.
	default:
		lite_assert(false);
		return xip_all_ones(ipv4);	// satifsy compiler
	}
}

XIPAddr xip_all_zeros(XIPVer ver)
{
	switch (ver)
	{
	case ipv4:
		return decode_ipv4addr_assertsuccess("0.0.0.0");
	case ipv6:
		return decode_ipv6addr_assertsuccess("::");
	default:
		lite_assert(false);
		return xip_all_ones(ipv4);	// satifsy compiler
	}
}

XIPAddr xip_broadcast(XIPAddr *subnet, XIPAddr *netmask)
{
	lite_assert(netmask->version == subnet->version);
	XIPAddr result;
	result.version = subnet->version;
	switch (subnet->version)
	{
	case ipv4:
		result.xip_addr_un.xv4_addr =
			subnet->xip_addr_un.xv4_addr
			| ~(netmask->xip_addr_un.xv4_addr);
		break;
	case ipv6:
	{
		int i;
		for (i=0; i<sizeof(result.xip_addr_un.xv6_addr.addr); i++)
		{
			result.xip_addr_un.xv6_addr.addr[i] =
				subnet->xip_addr_un.xv6_addr.addr[i]
				| ~(netmask->xip_addr_un.xv6_addr.addr[i]);
		}
		break;
	}
	default:
		lite_assert(false);
	}
	return result;
}

int XIPVer_to_idx(XIPVer ver)
{
	switch (ver)
	{
	case ipv4: return 0;
	case ipv6: return 1;
	default: lite_assert(false); return 0;
	}
}

XIPVer idx_to_XIPVer(int i)
{
	switch (i)
	{
	case 0: return ipv4;
	case 1: return ipv6;
	default: lite_assert(false); return ipv_err;
	}
}
