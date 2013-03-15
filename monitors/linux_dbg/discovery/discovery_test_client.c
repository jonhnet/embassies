#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdbool.h>

#include "discovery/discovery.h"

typedef struct {
	int socket;
} DiscoveryClient;

void dc_init(DiscoveryClient *dc, const char *bind_ifc)
{
	int rc;

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = 0;
	rc = inet_pton(AF_INET, bind_ifc, &saddr.sin_addr);
	assert(rc==1);

	dc->socket = socket(AF_INET, SOCK_DGRAM, 0);
	assert(dc->socket >= 0);

	int one = 1;
	rc = setsockopt(dc->socket, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
	assert(rc==0);

	rc = bind(dc->socket, (struct sockaddr*) &saddr, sizeof(saddr));
	assert(rc==0);
}

void dc_send_one(DiscoveryClient *dc)
{
	int rc;

	struct sockaddr_in daddr;
	daddr.sin_family = AF_INET;
	daddr.sin_port = htons(discovery_port);
	rc = inet_pton(AF_INET, "255.255.255.255", &daddr.sin_addr);
	assert(rc==1);

	char buf[300];
	memset(buf, 0x99, sizeof(buf));
	DiscoveryPacket *dp = (DiscoveryPacket *) buf;
	dp->magic = discovery_magic;
	void *data_ptr = &dp[1];

#define append(T) \
	{ \
		Discovery_datum_##T *tp = (Discovery_datum_##T*) data_ptr; \
		tp->hdr.datum_length = sizeof(Discovery_datum_##T); \
		tp->hdr.request_type = T; \
		/*fprintf(stderr, "@%6x Created %s, length 0x%02x\n", (uint32_t) data_ptr, #T, tp->hdr.datum_length);*/ \
		data_ptr += tp->hdr.datum_length; \
	}

	append(host_ifc_addr);
	append(host_ifc_app_range);
	append(router_ifc_addr);
	append(local_zftp_server_addr);

	int buflen = (data_ptr - (void*) buf);
	rc = sendto(dc->socket, buf, buflen, 0, (const struct sockaddr*) &daddr, sizeof(daddr));
	perror("sendto");
	assert(rc==buflen);
}

void dc_recv_one(DiscoveryClient *dc)
{
	int rc;
	char buf[300];
	rc = recv(dc->socket, buf, sizeof(buf), 0);
	if (rc<=0)
	{
		fprintf(stderr, "recv returned %d\n", rc);
		assert(false);
	}
}

int main(int argc, char **argv)
{
	DiscoveryClient dc;

	assert(argc==2);
	char *bind_ifc = argv[1];
	dc_init(&dc, bind_ifc);

	dc_send_one(&dc);
	dc_recv_one(&dc);

	return 0;
}
