#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdbool.h>

#include "discovery_server_args.h"
#include "discovery/discovery.h"
#include "xax_network_utils.h"

typedef struct {
	int socket;

	Args args;
} DiscoveryServer;

void ds_init(DiscoveryServer *ds)
{
	int rc;

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(discovery_port);
	saddr.sin_addr.s_addr = get_ip4(&ds->args.server_listen_ifc);

	ds->socket = socket(AF_INET, SOCK_DGRAM, 0);
	assert(ds->socket >= 0);

	int one = 1;
	rc = setsockopt(ds->socket, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
	assert(rc==0);

	rc = bind(ds->socket, (struct sockaddr*) &saddr, sizeof(saddr));
	if (rc!=0) { perror("bind"); }
	assert(rc==0);
}

bool ds_handle_one(DiscoveryServer *ds)
{
	int rc;
	char buf[300];
	struct sockaddr_in src_addr;
	socklen_t addrlen;

	addrlen = sizeof(src_addr);
	int packetlen;
	packetlen = recvfrom(ds->socket, buf, sizeof(buf), 0,
		(struct sockaddr*) &src_addr, &addrlen);
	assert(addrlen==sizeof(src_addr));
	if (packetlen<=0)
	{
		return false;
	}

	DiscoveryPacket *dp = (DiscoveryPacket *) buf;
	if (dp->magic != discovery_magic)
	{
		fprintf(stderr, "Invalid magic in discovery packet; dropping.\n");
		goto drop;
	}

	void *buf_ptr = (void*) &dp[1];
	void *buf_end = buf+packetlen;
	while (buf_ptr < buf_end)
	{
		DiscoveryDatum *dd = (DiscoveryDatum *) buf_ptr;
		//fprintf(stderr, "looking at datum len %x\n", dd->datum_length);
		if (buf_ptr + dd->datum_length > buf_end)
		{
			fprintf(stderr, "datum length %x, but only %x bytes remain; dropping\n",
				dd->datum_length, buf_end-buf_ptr);
			goto drop;
		}
		switch (dd->request_type)
		{
		case host_ifc_addr:
		{
			Discovery_datum_host_ifc_addr *dt = (Discovery_datum_host_ifc_addr *) dd;
			dt->host_ifc_addr = get_ip4(&ds->args.host_ifc_addr);
			break;
		}
		case host_ifc_app_range:
		{
			Discovery_datum_host_ifc_app_range *dt = (Discovery_datum_host_ifc_app_range *) dd;
			dt->host_ifc_app_range_low = get_ip4(&ds->args.host_ifc_app_range_low);
			dt->host_ifc_app_range_high = get_ip4(&ds->args.host_ifc_app_range_high);
			break;
		}
		case router_ifc_addr:
		{
			Discovery_datum_router_ifc_addr *dt = (Discovery_datum_router_ifc_addr *) dd;
			dt->router_ifc_addr = get_ip4(&ds->args.router_ifc_addr);
			break;
		}
		case local_zftp_server_addr:
		{
			Discovery_datum_local_zftp_server_addr *dt = (Discovery_datum_local_zftp_server_addr *) dd;
			dt->local_zftp_server_addr = get_ip4(&ds->args.local_zftp_server_addr);
			break;
		}
		default:
			fprintf(stderr, "(offset %x) invalid datum type %x; dropping.\n",
				buf_ptr-(void*)buf, dd->request_type);
			goto drop;
		}
		buf_ptr += dd->datum_length;
	}

	// we populated all the answer fields in the request. send it back!
	rc = sendto(ds->socket, buf, packetlen, 0,
		(struct sockaddr*) &src_addr, addrlen);
	assert(rc==packetlen);

drop:
	return true;
}

int main(int argc, char **argv)
{
	DiscoveryServer ds;
	args_init(&ds.args, argc, argv);

	ds_init(&ds);

	while (1)
	{
		bool rc = ds_handle_one(&ds);
		if (!rc)
		{
			break;
		}
	}

	return 0;
}
