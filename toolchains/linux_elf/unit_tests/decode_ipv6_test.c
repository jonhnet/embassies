#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <assert.h>

#include "xax_network_utils.h"
#include "format_ip.h"

void test_r(char *buf, bool expect_success, bool recurse)
{
	struct in6_addr sia;
	int control_rv = inet_pton(AF_INET6, buf, &sia);
	bool control_rc = (control_rv==1);

	XIPAddr exp_out;
	const char *failure = decode_ipv6addr(buf, &exp_out);
	bool exp_rc = (failure==NULL);
	
	char *indent = recurse ? "" : "    ";
	fprintf(stderr, "%s%s ctrl %d experiment %d failure %s\n",
		indent, buf, control_rc, exp_rc, failure);
	if (failure==NULL)
	{
		char fmt_buf[100];
		format_ip(fmt_buf, sizeof(fmt_buf), &exp_out);
		fprintf(stderr, "%s  %s\n", indent, fmt_buf);
		if (recurse)
		{
			test_r(fmt_buf, true, false);
		}
	}

	assert(control_rc==exp_rc);
	assert(control_rc==expect_success);
	if (expect_success)
	{
		assert(exp_out.version==ipv6);
		assert(memcmp(&exp_out.xip_addr_un.xv6_addr, sia.s6_addr, sizeof(exp_out.xip_addr_un.xv6_addr))==0);
	}
}

void test(char *buf, bool expect_success)
{
	test_r(buf, expect_success, true);
}

int main()
{
	test("::7:8:9", true);
		// read as "<colon>:7:8:9"
	test(":7:8:9", false);
	test("7:8:9", false);
	test("7::8:9", true);
		// read as "7:<epsilon>:8:9"
	test("7:8::9", true);
	test("7:8:9:", false);
	test("7:8:9::", true);

	test("0a.0.0.0", false);
	test("0a1:7", false);
	test("0:7", false);
	test("0::7", true);
	test("0.0.0.0", false);
	test("255.255.255.255", false);
	test("255.255.255.255.255", false);
	test("255.255.255", false);
	test("", false);
	test("0", false);
	test("::", true);
	test("0102:0304:0506:0708:090a:0b0c:0d0e:0f10:ab", false);
	test("0102:0304:0506:0708:090a:0b0c:0d0e:0f10", true);
	test("0102:0304:0506:0708:090a:0b0c:0d0e:0f10:", false);
	test("::0708:090a:0b0c:0d0e:0f10", true);
	test("0102:0304:0506::", true);
	test("0102::0b0c:0d0e:0f10", true);
	test("0:1:0:2:0::3", true);
	test("1:0:2:0::3", true);

	return 0;
}
