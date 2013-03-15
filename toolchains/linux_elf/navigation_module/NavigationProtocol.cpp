#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <netinet/in.h>

#include "NavigationProtocol.h"
#include "LiteLib.h"
#include "xax_network_utils.h"
#include "AppIdentity.h"

#if 0
NavigationProtocol::NavigationProtocol(ZoogDispatchTable_v1 *zdt)
{
	XIPifconfig ifconfigs[2];
	int count = 2;
	(zdt->zoog_get_ifconfig)(ifconfigs, &count);
	bool found = false;
	for (int i=0; i<count; i++)
	{
		if (ifconfigs[i].local_interface.version == ipv4)
		{
			local_interface = ifconfigs[i].local_interface;
			broadcast = xip_broadcast(
				&ifconfigs[i].local_interface, &ifconfigs[i].netmask);
			found = true;
			break;
		}
	}
	lite_assert(found);	// cannot find address
}

#if 0
void NavigationProtocol::address_msg_to_vendor_self(NavigationBlob *msg)
{
	// "address" this packet to myself, for now.
	AppIdentity ai;
	ZPubKey *dest_ident = ai.get_pub_key();
	address_msg_to_vendor(msg, dest_ident);
}

void NavigationProtocol::address_msg_to_vendor(NavigationBlob *msg, ZPubKey *dest_ident)
{
	NavigationBlob recipient_blob;
	uint8_t *buf = recipient_blob.write_in_place(dest_ident->size());
//recipient_blob.dbg_check_magic();
	dest_ident->serialize(buf);
	msg->append(&recipient_blob);
}
#endif

void NavigationProtocol::crude_send_to_app(NavigationBlob *msg)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	lite_assert(sock>=0);

	// TODO skipping crypto verification steps
	// send a broadcast looking for the vendor

	// verify the vendor by creating a stream

	// send the message to the vendor
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	//inet_pton(AF_INET, "10.0.0.1", &dest_addr.sin_addr);
	assert(sizeof(dest_addr.sin_addr.s_addr) == sizeof(broadcast.xip_addr_un.xv4_addr));
	memcpy(&dest_addr.sin_addr.s_addr,
		&broadcast.xip_addr_un.xv4_addr,
		sizeof(broadcast.xip_addr_un.xv4_addr));
	dest_addr.sin_port = htons(NavigationProtocol::PORT);

	uint32_t size = msg->size();
	void *bytes = msg->read_in_place(size);
	int flags = 0;
	int rc = sendto(sock, bytes, size, flags,
		(struct sockaddr *) &dest_addr, sizeof(dest_addr));
	lite_assert(rc==(int) size);
	close(sock);
}
#endif
