#include "LiteLib.h"
#include "cheesy_snprintf.h"
#include "format_ip.h"

void format_ip4(char *buf, int len, XIP4Addr *addr)
{
	cheesy_snprintf(buf, len, "%u.%u.%u.%u",
		((*addr) >> 0) & 0xff,
		((*addr) >> 8) & 0xff,
		((*addr) >>16) & 0xff,
		((*addr) >>24) & 0xff);
}

uint16_t _clump(XIP6Addr *addr, int idx)
{
	lite_assert(idx<8);
	return (addr->addr[idx*2]<<8) | addr->addr[idx*2+1];
}

void format_ip6(char *buf, int len, XIP6Addr *addr)
{
	char *p = buf;

	bool skipping = false;
	bool skipped = false;

	int l = len;
	int n = cheesy_snprintf(p, l, "%x", _clump(addr, 0));
	p+=n;
	l-=n;
	
	int i;
	for (i=1; i<8; i++)
	{
		if (i<7 && skipping)
		{
			if (_clump(addr, i)==0)
			{
				continue;
			}
			else
			{
				skipping = false;
				skipped = true;
				// finished our first skip range.
				// fall through and print this one.
			}
		}
		else
		{
			if (_clump(addr, i)==0)
			{
				if (!skipped)
				{
					skipping = true;
					n = cheesy_snprintf(p, l, ":");
					p += n;
					l -= n;
					continue;
				}
				else
				{
					// No more skipping allowed.
					// fall through and print this one.
				}
			}
			else
			{
				// just another number.
				// fall through and print this one.
			}
		}
		n = cheesy_snprintf(p, l, ":%x", _clump(addr,i));
		p += n;
		l -= n;
	}
}

void format_ip(char *buf, int len, XIPAddr *addr)
{
	switch (addr->version)
	{
	case ipv4:
		return format_ip4(buf, len, &addr->xip_addr_un.xv4_addr);
	case ipv6:
		return format_ip6(buf, len, &addr->xip_addr_un.xv6_addr);
	default:
		lite_assert(false);
	}
}


void format_netmask(char *buf, int len, XIPAddr *addr)
{
	int len_bits;
	switch (addr->version)
	{
	case ipv4:
		len_bits = 32;
		break;
	case ipv6:
		len_bits = 128;
		break;
	default:
		lite_assert(false);
		len_bits = 0;
	}

	uint8_t *bytes = (uint8_t*) &addr->xip_addr_un;

	int bit;
	int netmask = -1;
	for (bit=0; bit<len_bits; bit++)
	{
		bool bitvalue = (bytes[bit>>3] >> (7-(bit&0x07))) & 1;
		if (bitvalue==0)
		{
			if (netmask==-1)
			{
				netmask = bit;
			}
			else
			{
				// more zeros after the 1 prefix
			}
		}
		else
		{
			if (netmask==-1)
			{
				// more ones in the prefix
			}
			else
			{
				// oops -- found a 1 after a 0
				char tbuf[100];
				format_ip(tbuf, sizeof(tbuf), addr);
				cheesy_snprintf(buf, len, "bogus_netmask<%s>", tbuf);
				return;
			}
		}
	}
	if (netmask==-1)
	{
		netmask = len_bits;
	}
	cheesy_snprintf(buf, len, "/%d", netmask);
	return;
}
