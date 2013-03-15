#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include "Message.h"
#include "LiteLib.h"

Message::Message(int rendezvous_socket)
{
	lma = NULL;

	int capacity = ZK_SMALL_MESSAGE_LIMIT;
	read_buf = (char*) malloc(capacity);

	remote_addr_len = sizeof(remote_addr);
	int rc = recvfrom(rendezvous_socket, read_buf, capacity,
		0, (struct sockaddr *) remote_addr, &remote_addr_len);
	lite_assert(rc > 0);
	read_buf_size = rc;
}

Message::Message(void *payload, uint32_t size)
{
	// from tunnel; we want to prepend a CMDeliverPacket header
	// so it's ready to send out to a monitor.
	lma = NULL;

	read_buf_size = sizeof(CMDeliverPacket) + size;
	read_buf = (char*) malloc(read_buf_size);

	CMDeliverPacket *dp = (CMDeliverPacket *) read_buf;
	dp->hdr.length = read_buf_size;
	dp->hdr.opcode = co_deliver_packet;
	memcpy(dp->payload, payload, size);

	remote_addr_len = sizeof(remote_addr);

	lite_assert(read_buf_size > 0);
}

Message::~Message()
{
	if (lma!=NULL)
	{
		lma->unmap();
		// TODO drop the reference?
	}
	free(read_buf);
}

void *Message::get_payload()
{
	if (lma!=NULL)
	{
		return lma->map();
	}
	return read_buf;
}

uint32_t Message::get_payload_size()
{
	if (lma!=NULL)
	{
		return lma->get_mapped_size();
	}
	return read_buf_size;
}

void Message::ingest_long_message(LongMessageAllocator *allocator, CMLongMessage *cm_long)
{
	lma = allocator->ingest(&cm_long->id, cm_long->hdr.length - offsetof(CMLongMessage, id));
}

const char *Message::get_remote_addr()
{
	lite_assert(remote_addr_len > 0);	// hey, this Message didn't come from a monitor; it's a pseudo co_deliver_packet from a tunnel. What shenanigans are you trying to pull, pal-o?
	return remote_addr;
}

socklen_t Message::get_remote_addr_len()
{
	lite_assert(remote_addr_len > 0);
	return remote_addr_len;
}
