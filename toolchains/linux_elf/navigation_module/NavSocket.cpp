#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "xax_extensions.h"

#include "NavSocket.h"

NavSocket::NavSocket()
	: zdt(xe_get_dispatch_table())
{
	_lookup_addrs();
	_open_socket();
}

NavSocket::~NavSocket()
{
	close(socketfd);
}

void NavSocket::_lookup_addrs()
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

void NavSocket::_open_socket()
{
    // Open a UDP socket
    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(socketfd>=0);
    
    // Construct the sender address space and the recieve buffer
    sender_address_len = sizeof(sender_address);
    memset((char *) &sender_address, 0, sizeof(sender_address));
}
	
void NavSocket::send_and_delete_msg(NavigationBlob *msg)
{
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
	int rc = sendto(socketfd, bytes, size, flags,
		(struct sockaddr *) &dest_addr, sizeof(dest_addr));
	lite_assert(rc==(int) size);

	delete msg;
}

