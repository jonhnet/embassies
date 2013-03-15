#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <assert.h>

#include "xax_network_utils.h"

void test(char *buf, bool expect_success)
{
	struct in_addr sia;
	int control_rv = inet_pton(AF_INET, buf, &sia);
	bool control_rc = (control_rv==1);

	XIP4Addr exp_out;
	bool exp_rc = decode_ipv4addr(buf, &exp_out);
	
	fprintf(stderr, "%s\n", buf);
	assert(control_rc==exp_rc);
	assert(control_rc==expect_success);
	if (expect_success)
	{
		assert(exp_out == sia.s_addr);
	}
}

int main()
{
	test("0.0.0.0", true);
	test("255.255.255.255", true);
	test("255.255.255.255.255", false);
	test("255.255.255", false);
	test("", false);
	test("0", false);

	int i;
	for (i=0; i<100; i++)
	{
		bool expect_success = true;
		char buf[100];
		uint32_t octet[4];
		int oi;
		for (oi=0; oi<4; oi++)
		{
			octet[oi] = rand() & 0xff;
		}
		bool dec_error = (rand() & 0x3) == 0;
		if (dec_error)
		{
			oi = rand() & 0x3;
			octet[oi] += 256;
			expect_success = false;
		}
		sprintf(buf, "%d.%d.%d.%d",
			octet[0],
			octet[1],
			octet[2],
			octet[3]);
		bool syntax_error = (rand() & 0x3) == 0;
		if (syntax_error)
		{
			int position = rand() % strlen(buf);
			buf[position] = ';';
			expect_success = false;
		}

		test(buf, expect_success);
	}
	return 0;
}
