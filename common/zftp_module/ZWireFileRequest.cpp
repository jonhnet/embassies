#include "ZWireFileRequest.h"

ZWireFileRequest::ZWireFileRequest(
	MallocFactory *mf,
	ZFileServer *zfileserver,
	UDPEndpoint *remote_addr,
	uint8_t *bytes,
	uint32_t len)
	: ZFileRequest(mf, bytes, len)
{
	this->zfileserver = zfileserver;
	this->fromlen = fromlen;
	this->remote_addr = *remote_addr;
	this->result = NULL;
}

ZWireFileRequest::~ZWireFileRequest()
{
}

void ZWireFileRequest::deliver_result(InternalFileResult *result)
{
	lite_assert(this->result==NULL);
	this->result = result;

#if 0
	BlockSet set = *get_block_set();
	if (remote_addr.ipaddr.version==ipv4 && set.size() > 1)
	{
		// we're a wire request. Give 'im only one packet for now.
		set = BlockSet(set.first(), set.first());
	}
	// ipv6 can receive as much data as it durn well pleases.
#endif

	zfileserver->send_reply(this, result->get_reply_size(this));
}

void ZWireFileRequest::fill_payload(Buf* vbuf)
{
	result->fill_reply(vbuf, this);
}
