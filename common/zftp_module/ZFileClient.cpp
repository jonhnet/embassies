#include "ZFileClient.h"
#include "ZFileReplyFromServer.h"
#include "ZCache.h"
#include "zlc_util.h"
#include "xax_network_utils.h"

ZFileClient::ZFileClient(ZCache *zcache, UDPEndpoint *origin_zftp, SocketFactory *sockf, ThreadFactory *tf, uint32_t max_payload)
{
	this->zcache = zcache;
	this->ze = zcache->GetZLCEmit();
	this->origin_zftp = origin_zftp;
	this->sock = sockf->new_socket(
		sockf->get_inaddr_any(origin_zftp->ipaddr.version), false);
	this->_max_payload = max_payload;
	lite_assert(this->sock!=NULL);
	tf->create_thread(_run_trampoline, this, 32<<10);
}

void ZFileClient::_run_trampoline(void *v_this)
{
	ZFileClient *zfc = (ZFileClient *) v_this;
	zfc->run_worker();
}

ZFileReplyFromServer *ZFileClient::_receive()
{
	ZeroCopyBuf *zcb = sock->recvfrom(NULL);
	if (zcb==NULL)
	{
		//perror("recv");
		lite_assert(false);	// recv failed
	}
	ZLC_CHATTY(ze, "ZFileClient::_receive got %d bytes\n",, zcb->len());
	ZFileReplyFromServer *zreply = new ZFileReplyFromServer(zcb, ze);
	if (!zreply->is_valid())
	{
		ZLC_CHATTY(ze, "ZFileClient::_receive ...but they weren't valid.\n");
		delete zreply;
		return NULL;
	}

	ZLC_CHATTY(ze, "ZFileClient::_receive ...valid & processing.\n");
	return zreply;
}

void ZFileClient::run_worker()
{
	zcache->wait_until_configured();

	while (1)
	{
		ZFileReplyFromServer *reply = _receive();
		if (reply==NULL)
		{
			continue;
		}
		lite_assert(reply->is_valid());
		ZCachedFile *zcf = zcache->lookup_hash(reply->get_filehash());
		zcf->install_reply(reply);
		delete reply;
	}
}

void ZFileClient::issue_request(OutboundRequest *obr)
{
	bool rc = sock->sendto(origin_zftp, obr->get_zrp(), obr->get_len());
	if (!rc)
	{
		//perror("sendto failed");
		ZLC_COMPLAIN(ze, "sendto failed\n");
	}
	delete obr;
}

uint32_t ZFileClient::get_max_payload()
{
	return _max_payload;
}
