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
	this->sock = sockf->new_socket(NULL, origin_zftp, false);
		//sockf->get_inaddr_any(origin_zftp->ipaddr.version), false);
	this->_max_payload = max_payload;
	lite_assert(this->sock!=NULL);
	tf->create_thread(_run_trampoline, this, 32<<10);
	
	this->dbg_zdt = NULL;
	this->dbg_bw_started = false;
	this->dbg_total_payload = 0;

}

void ZFileClient::dbg_enable_bandwidth_reporting(ZoogDispatchTable_v1* dbg_zdt)
{
	this->dbg_zdt = dbg_zdt;
}

void ZFileClient::_run_trampoline(void *v_this)
{
	ZFileClient *zfc = (ZFileClient *) v_this;
	zfc->run_worker();
}

void ZFileClient::dbg_bw_measure(uint32_t payload_len)
{
	// NB assumes every packet we receive is useful.

	// NB depends on having a ZDT; doesn't work in posix builds.
	if (dbg_zdt == NULL)
	{
		return;
	}

	if (!dbg_bw_started)
	{
		(dbg_zdt->zoog_get_time)(&dbg_start_time);
		dbg_bw_started = true;
	}
	dbg_total_payload += payload_len;
	uint64_t dbg_cur_time;
	(dbg_zdt->zoog_get_time)(&dbg_cur_time);

	double elapsed_secs = (dbg_cur_time-dbg_start_time)/1000000000.0;
	double bytes_per_sec = ((double)dbg_total_payload) / elapsed_secs;
	double megabits_per_sec = bytes_per_sec*8/(1024*1024);

	ZLC_TERSE(ze, "bw +%d = %d bytes, %.1f Mbps elapsed %d\n",,
		payload_len, dbg_total_payload, megabits_per_sec, (int) elapsed_secs);
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
	ZFileReplyFromServer *zreply = new ZFileReplyFromServer(zcb, ze, zcache->get_compression());
	if (!zreply->is_valid())
	{
		ZLC_CHATTY(ze, "ZFileClient::_receive ...but they weren't valid.\n");
		delete zreply;
		return NULL;
	}
	dbg_bw_measure(zreply->get_payload_len());

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
