#include "LiteLib.h"
#include "ZFileServer.h"
#include "ZWireFileRequest.h"
#include "ZCachedFile.h"
#include "ZCache.h"
#include "zlc_util.h"
#include "sockaddr_util.h"
#include "ValidatedZCB.h"
#include "CompressedZCB.h"
#include "MemBuf.h"

#define MIN_CACHE_SIZE	(1<<20)
	// Only play the memcpy-avoiding caching game with packets at least
	// 1MB in size. The expedient that defers the memcpy for a couple seconds
	// plays havoc with the main loop when we have a bunch of RTTs!

// Implements a poor man's CoW:
// On 1st request: Copy reply to a "ready" copy and to a pristine master copy
// When a new request comes in, immediately give it the "ready to serve" version
// and start a new copy from the master

ZFileServer::ZFileServer(ZCache *zcache, UDPEndpoint *listen_zftp, SocketFactory *sf, ThreadFactory *tf, PerfMeasureIfc* pmi)
	: use_zero_copy_reply_optimization(true),
	  cached_req(NULL),
	  cached_vbuf(NULL),
	  pmi(pmi)
{
	lite_assert(listen_zftp!=NULL);

	this->zcache = zcache;
	this->socket = sf->new_socket(listen_zftp, NULL, false);
	tf->create_thread(_run_trampoline, this, 128<<10);
}

void ZFileServer::_run_trampoline(void *v_this)
{
	ZFileServer *zfs = (ZFileServer *) v_this;
	zfs->run_worker();
}

ZFileRequest *ZFileServer::_receive()
{
	UDPEndpoint remote_addr;

	ZeroCopyBuf *zcb = socket->recvfrom(&remote_addr);
	lite_assert(zcb!=NULL);

	ZFileRequest *req = new ZWireFileRequest(zcache->mf, this, &remote_addr, zcb->data(), zcb->len());
	delete zcb;
	return req;
}

void ZFileServer::run_worker()
{
	zcache->wait_until_configured();

	while (1)
	{
		ZFileRequest *request = _receive();
		if (!request->parse(zcache->GetZLCEmit(), false))
		{
			ZLC_COMPLAIN(zcache->GetZLCEmit(), "(ZFileServer) Malformed client request; dropping.\n");
			delete request;
			continue;
		}
		pmi->mark_time("received a request");

		bool was_fast = false;
		if (use_zero_copy_reply_optimization)
		{
			was_fast = zero_copy_reply_optimization(request);
		}
		if (was_fast)
		{
			delete request;
			continue;
		}

		ZCachedFile *zcf = zcache->lookup_hash(request->get_hash());
		zcf->consider_request(request);
	}
}

bool ZFileServer::req_is_cachable(ZFileRequest *req)
{
	return use_zero_copy_reply_optimization
		&& !req->get_data_range().is_empty()
		&& req->get_compression_context().state_id==ZFTP_NO_COMPRESSION;
}

void ZFileServer::send_reply(ZFileRequest *req, uint32_t payload_len)
	// NB "payload" here refers to the entire ZDP and its zftp payload,
	// but excludes any IP/UDP headers.
{
	ZWireFileRequest *creq = (ZWireFileRequest *) req;
	Buf* vbuf = NULL;
	ZeroCopyBuf* zcb = NULL;

//	bool debug_diagnose_bogus_zarfile = false;

	if (cached_vbuf != NULL && 
			cached_vbuf->get_zcb()->len() >= payload_len &&
			payload_len >= MIN_CACHE_SIZE) {
		// Recycle the vbuf we already have
		ZLC_COMPLAIN(zcache->GetZLCEmit(), "using recycled vbuf\n");
//		debug_diagnose_bogus_zarfile = true;
		vbuf = cached_vbuf;
		((ValidatedZCB*) vbuf)->recycle();
		zcb = cached_vbuf->get_zcb();
		cached_vbuf = NULL;
	} else {
		if (cached_vbuf!=NULL)
		{
			ZLC_COMPLAIN(zcache->GetZLCEmit(),
				"Can't recycle. cached_vbuf len %d, payload_len %d, MIN %d\n",,
				cached_vbuf->get_zcb()->len(), payload_len, MIN_CACHE_SIZE);
		}
		if (req->get_compression_context().state_id == ZFTP_NO_COMPRESSION)
		{
			zcb = socket->zc_allocate(payload_len);
			vbuf = new ValidatedZCB(zcache, zcb, socket);
		}
		else
		{
			ZStreamIfc* zstream =
				zcache->get_compression()
					->get_stream(req->get_compression_context(), ZCompressionIfc::COMPRESS);
			zcb = socket->zc_allocate(zstream->stretch(payload_len));
			vbuf = zcache->get_compression()->make_compressed_zcb(
				zcb, socket, zcache->mf, zstream);
		}
	}
	creq->fill_payload(vbuf);

//	if (debug_diagnose_bogus_zarfile)
//	{
//		uint8_t* buf = (uint8_t*) mf_malloc(zcache->mf, payload_len);
//		MemBuf mb(buf, payload_len);
//		creq->fill_payload(&mb);
//		lite_assert(zcb->len() == payload_len);
//		bool result = memcmp(zcb->data(), buf, payload_len)==0;
//		ZLC_COMPLAIN(zcache->GetZLCEmit(),
//			"After sloooowly comparing both versions, match = %d\n",,
//			result);
//		mf_free(zcache->mf, buf);
//	}

	bool rc = socket->zc_send(&creq->remote_addr, zcb, vbuf->get_packet_len());
	if (rc)
	{
		//ZLC_COMPLAIN(zcache->GetZLCEmit(), "Workin' for a living.\n");
//		vbuf->printDebugStats();
		pmi->mark_time("sent a constructed reply (slow path)");
	}
	else
	{
		ZLC_COMPLAIN(zcache->GetZLCEmit(), "bummer, send of reply failed.\n");
	}
	delete vbuf;

	cache_or_discard_result(req, payload_len);
}

void ZFileServer::cache_or_discard_result(ZFileRequest *req, uint32_t payload_len)
{
	if (req_is_cachable(req) && payload_len>=MIN_CACHE_SIZE)
	{
		unprincipled_sleep_to_emulate_asynchrony();
		ZWireFileRequest *creq = (ZWireFileRequest *) req;

		if (cached_vbuf != NULL)
		{
			// cache eviction for a 1-line cache
			delete cached_vbuf;
			cached_vbuf = NULL;
			lite_assert(cached_req!=NULL);
			delete cached_req;
			cached_req = NULL;
		}

		// Pre-build another copy in case the next request is hot.
		ZeroCopyBuf* cached_zcb = socket->zc_allocate(payload_len+fast_case_cushion);
		cached_vbuf = new ValidatedZCB(zcache, cached_zcb, socket);

		creq->fill_payload(cached_vbuf);
		cached_req = req;
		cached_payload_len = payload_len;
	}
	else
	{
		delete req;
	}
}

void ZFileServer::unprincipled_sleep_to_emulate_asynchrony()
{
	// Get off the critical path
	SyncFactoryEvent* sleepTimer = zcache->sf->new_event(false);
	sleepTimer->wait(2*1000);  // Sleep for 2 seconds (in ms)
	delete sleepTimer;
}

bool ZFileServer::zero_copy_reply_optimization(ZFileRequest *req)
{
	if (cached_req==NULL)
	{
		return false;
	}

	if (!req->is_semantically_equivalent(cached_req))
	{
		return false;
	}

	ZWireFileRequest *creq = (ZWireFileRequest *) req;
	bool rc = socket->zc_send(&creq->remote_addr, cached_vbuf->get_zcb(), cached_vbuf->get_packet_len());
	if (!rc)
	{
		ZLC_COMPLAIN(zcache->GetZLCEmit(), "bummer, send of (cached) reply failed.\n");
	}
	else
	{
		ZLC_COMPLAIN(zcache->GetZLCEmit(), "Cached send! Req %x-%x, reply %x-%x\n",,
			req[0].zrp[0].data_start, req[0].zrp[0].data_end,
			((ZFTPDataPacket*)cached_vbuf->get_zcb()->data())[0].data_start,
			((ZFTPDataPacket*)cached_vbuf->get_zcb()->data())[0].data_end);
		pmi->mark_time("sent a pre-duplicated reply (fast path)");
	}
	delete cached_vbuf;
	cached_vbuf = NULL;
	ZFileRequest* the_cached_req = cached_req;
	cached_req = NULL;

	cache_or_discard_result(the_cached_req, cached_payload_len);

	return true;
}

