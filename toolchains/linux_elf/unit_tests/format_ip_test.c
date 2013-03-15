#include <stdio.h>
#include <alloca.h>
#include "format_ip.h"
#include "xax_network_utils.h"

void test_one(XIPAddr *a, int len)
{
	char *buf = alloca(len);
	format_ip(buf, len, a);
	fprintf(stderr, "result: '%s'\n", buf);
}

void test_lens(XIPAddr *a)
{
	test_one(a, 200);
	test_one(a, 12);
	test_one(a, 7);
	test_one(a, 1);
}

void test_mask(const char *desc, int bit, XIPAddr *a)
{
	char buf[200];
	format_netmask(buf, sizeof(buf), a);
	fprintf(stderr, "mask %d, %s: %s\n", bit, desc, buf);
}

int main()
{
	{
		XIP4Addr a4 = literal_ipv4addr(127,0,0,1);
		XIPAddr a = ip4_to_xip(&a4);
		test_lens(&a);
	}
	{
		XIP4Addr a4 = literal_ipv4addr(0,0,0,5);
		XIPAddr a = ip4_to_xip(&a4);
		test_lens(&a);
	}
	{
		XIP4Addr a4 = literal_ipv4addr(255,255,0,0);
		XIPAddr a = ip4_to_xip(&a4);
		test_lens(&a);
	}

	XIPAddr a6_zero;
	a6_zero.version = ipv6;
	lite_memset(a6_zero.xip_addr_un.xv6_addr.addr, 0, sizeof(a6_zero.xip_addr_un.xv6_addr.addr));

	{
		XIPAddr a6 = a6_zero;
		lite_memset(a6.xip_addr_un.xv6_addr.addr, 0x17, sizeof(a6.xip_addr_un.xv6_addr.addr));
		test_lens(&a6);
	}
	{
		XIPAddr a6 = a6_zero;
		test_lens(&a6);
	}
	int i;
	for (i=0; i<16; i++)
	{
		XIPAddr a6 = a6_zero;
		a6.xip_addr_un.xv6_addr.addr[i] = 0x19;
		test_lens(&a6);
	}
	for (i=0; i<16; i++)
	{
		int j;
		for (j=0; j<16; j++)
		{
			XIPAddr a6 = a6_zero;
			a6.xip_addr_un.xv6_addr.addr[i] = 0x19;
			a6.xip_addr_un.xv6_addr.addr[j] = 0x1a;
			test_one(&a6, 200);
		}
	}

	for (i=0; i<=128; i++)
	{
		XIPAddr a;
		lite_memset(&a, 0, sizeof(a));
		uint8_t *bytes = (uint8_t*) &a.xip_addr_un;
		int bit;
		for (bit=0; bit<i; bit++)
		{
			bytes[bit>>3] |= (1<<(7-(bit&7)));
		}
		a.version = ipv4;
		test_mask("v4", i, &a);
		a.version = ipv6;
		test_mask("v6", i, &a);

		int tamperbit = bit-2;
		bytes[tamperbit>>3] &= ~(1<<(7-((tamperbit)&7)));
		a.version = ipv6;
		test_mask("v6-tampered", i, &a);
	}

	{
		XIPAddr f;
		//f.version = ipv6;
		//lite_memset(&f.xip_addr_un.xv6_addr, 0xff, sizeof(f.xip_addr_un.xv6_addr));
		f = get_ip_subnet_broadcast_address(ipv6);
		test_one(&f, 200);
	}

	return 0;
}

