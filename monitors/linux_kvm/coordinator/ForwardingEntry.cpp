#include "xax_network_utils.h"
#include "format_ip.h"
#include "ForwardingEntry.h"

ForwardingEntry::ForwardingEntry(NetIfc *net_ifc, XIPAddr addr, XIPAddr netmask, const char *name)
{
	this->net_ifc = net_ifc;
	this->addr = addr;
	this->netmask = netmask;
	this->name = name;
}

bool ForwardingEntry::_buf_mask(void *vb0, void *vb1, void *vmask, int size)
{
	lite_assert((size & 0x3) == 0);
	int words = size>>2;
	uint32_t *b0 = (uint32_t *) vb0;
	uint32_t *b1 = (uint32_t *) vb1;
	uint32_t *mask = (uint32_t *) vmask;
	int i;
	for (i=0; i<words; i++)
	{
		if ((b0[i] & mask[i]) != (b1[i] & mask[i]))
		{
			return false;
		}
	}
	return true;
}

bool ForwardingEntry::mask_match(XIPAddr *test_addr)
{
	if (addr.version==ipv4)
	{
		if (test_addr->version!=ipv4)
		{
			return false;
		}
		lite_assert(addr.version==ipv4);
		lite_assert(test_addr->version==ipv4);
		lite_assert(netmask.version==ipv4);
		return _buf_mask(
			&addr.xip_addr_un.xv4_addr,
			&test_addr->xip_addr_un.xv4_addr,
			&netmask.xip_addr_un.xv4_addr,
			sizeof(addr.xip_addr_un.xv4_addr));
	}
	else
	{
		if (test_addr->version!=ipv6)
		{
			return false;
		}
		lite_assert(addr.version==ipv6);
		lite_assert(test_addr->version==ipv6);
		lite_assert(netmask.version==ipv6);
		return _buf_mask(
			&addr.xip_addr_un.xv6_addr,
			&test_addr->xip_addr_un.xv6_addr,
			&netmask.xip_addr_un.xv6_addr,
			sizeof(addr.xip_addr_un.xv6_addr));
	}
}

bool ForwardingEntry::equals(ForwardingEntry *other)
{
	return cmp_xip(&addr, &other->addr)==0
		&& cmp_xip(&netmask, &other->netmask)==0;
}

void ForwardingEntry::print(char *buf, int bufsiz)
{
	char s_dest[100], s_mask[100];
	format_ip(s_dest, sizeof(s_dest), &addr);
	format_netmask(s_mask, sizeof(s_mask), &netmask);
	snprintf(buf, bufsiz, "%40s %10s %s",
		s_dest, s_mask, name);
}
