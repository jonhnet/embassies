#include "LiteLib.h"
#include "xax_network_utils.h"
#include "ZLookupRequest.h"
#include "ZLookupServer.h"

ZLookupRequest::ZLookupRequest(
	ZLookupServer *zlookup_server,
	UDPEndpoint *remote_addr,
	uint8_t *bytes,
	uint32_t len,
	MallocFactory *mf)
{
	this->zlookup_server = zlookup_server;
	this->fromlen = fromlen;
	this->remote_addr = *remote_addr;
	this->len = len;
	this->bytes = (uint8_t*) mf_malloc(mf, len);
	this->mf = mf;
	lite_memcpy(this->bytes, bytes, len);
}

ZLookupRequest::~ZLookupRequest()
{
	mf_free(mf, bytes);
}

bool ZLookupRequest::parse()
{
	zhlrp = (ZFTPHashLookupRequestPacket *) bytes;
	nonce = Z_NTOHG(zhlrp->nonce);
	url_hint_len = Z_NTOHG(zhlrp->url_hint_len);
	if (sizeof(ZFTPHashLookupRequestPacket)+url_hint_len > len)
	{
		return false;
	}
	url_hint = (char*) (&zhlrp[1]);
	if (url_hint[url_hint_len-1] != '\0')
	{
		return false;
	}
	return true;
}

void ZLookupRequest::deliver_reply(ZNameRecord *znr)
{
#if ZLC_INCLUDE_SERVERS
	zlookup_server->send_reply(this, znr);
#endif // ZLC_INCLUDE_SERVERS
}
