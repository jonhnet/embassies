#include "xax_network_utils.h"
#include "zlc_util.h"
#include "sockaddr_util.h"
#include "ZLookupServer.h"
#include "ZCache.h"

ZLookupServer::ZLookupServer(ZCache *zcache, UDPEndpoint *listen_lookup, ZLCEmit *log_paths_emitter, SocketFactory *sf, ThreadFactory *tf)
{
	lite_assert(listen_lookup!=NULL);

	this->zcache = zcache;
	this->ze = zcache->GetZLCEmit();
	this->socket = sf->new_socket(listen_lookup, NULL, false);

	this->log_paths_emitter = log_paths_emitter;

	tf->create_thread(_run_inner_s, this, 32<<10);
}

ZLookupRequest *ZLookupServer::_receive()
{
	UDPEndpoint remote_addr;
		// TODO if we ever use TCP comm, we'll want to rename UDPEndpoint
		// to capture the fact that it's a generic endpoint
		
	ZeroCopyBuf *zcb = socket->recvfrom(&remote_addr);

	ZLookupRequest *req = new ZLookupRequest(this, &remote_addr, zcb->data(), zcb->len(), zcache->mf);
	delete zcb;
	return req;
}

void ZLookupServer::_run_inner_s(void *v_this)
{
	((ZLookupServer*) v_this)->_run_inner();
}

void ZLookupServer::_run_inner()
{
	zcache->wait_until_configured();

	while (1)
	{
		ZLookupRequest *request = _receive();
		if (!request->parse())
		{
			ZLC_COMPLAIN(ze, "(ZLookupServer) Malformed client request; dropping.\n");
			delete request;
			continue;
		}

		ZCachedName *zcn = zcache->lookup_url(request->url_hint);
		zcn->consider_request(request);
	}
}

void ZLookupServer::send_reply(ZLookupRequest *req, ZNameRecord *name_record)
{
	ZFTPHashLookupReplyPacket reply;
	reply.nonce = z_htong(req->nonce, sizeof(reply.nonce));
	reply.hash_len = z_htong(sizeof(hash_t), sizeof(reply.hash_len));

	if (name_record!=NULL)
	{
		reply.hash = *name_record->get_file_hash();
		const char *disposition_label = 
			lite_memequal((const char*) &reply.hash, 0, sizeof(reply.hash))
				? "ENOENT"
				: "found ";
		(void) disposition_label;	// in case it's compiled out.
		ZLC_TERSE(ze, "%s %s\n",, disposition_label, req->url_hint);

		if (log_paths_emitter != NULL)
		{
			log_paths_emitter->emit(req->url_hint);
			log_paths_emitter->emit("\n");
		}
	}
	else
	{
		ZLC_COMPLAIN(ze, "Lookup fails for %s\n",, req->url_hint);
		lite_memset(&reply.hash, 0, sizeof(reply.hash));
	}

	bool rc;
	rc = socket->sendto(&req->remote_addr, &reply, sizeof(reply));
	if (!rc)
	{
		ZLC_COMPLAIN(ze, "(lookup) bummer, send of reply failed.\n");
	}

	delete req;
}
