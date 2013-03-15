#include <malloc.h>
#include "CoordinatorConnection.h"
#include "RPCToken.h"

RPCToken::RPCToken(CMRPCHeader *req, CMRPCHeader *reply, CoordinatorConnection *cc)
	: nonce(req->rpc_nonce),
	  req(req),
	  reply(reply),
	  full_reply(NULL),
	  cc(cc),
	  event(cc->_get_sf()->new_event(false))
{
}

RPCToken::RPCToken(uint32_t nonce)
	: nonce(nonce),
	  req(NULL),
	  reply(NULL),
	  full_reply(NULL),
	  cc(NULL),
	  event(NULL)
{
}

RPCToken::~RPCToken()
{
	if (full_reply)
	{
		free(full_reply);
		full_reply = (CMRPCHeader*) 0x11111111;	// catch use-after-free
	}
	delete event;
}

void RPCToken::wait()
{
	cc->_send(req, req->hdr.length);
	event->wait();
}

void RPCToken::transact()
{
	wait();
	delete this;
}

CMRPCHeader *RPCToken::get_full_reply()
{
	if (full_reply==NULL)
	{
		// turns out the reply fit in the budgeted space; just return that.
		return reply;
	}
	return full_reply;
}

void RPCToken::reply_arrived(CMRPCHeader *rpc)
{
	// this->full_reply = rpc;
		// that's not legal, because we don't own rpc object. Is making
		// a copy sane?
	lite_assert(reply->hdr.length <= rpc->hdr.length);
	lite_assert(reply->hdr.opcode == rpc->hdr.opcode);
	lite_assert(reply->rpc_nonce == rpc->rpc_nonce);

	if (reply->hdr.length < rpc->hdr.length)
	{
		// This message was longer than we made space for;
		// make a copy of the full data available.
		full_reply = (CMRPCHeader*) malloc(rpc->hdr.length);
		memcpy(full_reply, rpc, rpc->hdr.length);
	}

	// make a copy in the stack-local that provides easy syntax for
	// most funcs
	memcpy(reply, rpc, reply->hdr.length);
	event->signal();
}

uint32_t RPCToken::hash(const void *v_a)
{
	RPCToken* a = (RPCToken*) v_a;
	return a->nonce;
}

int RPCToken::cmp(const void *v_a, const void *v_b)
{
	RPCToken* a = (RPCToken*) v_a;
	RPCToken* b = (RPCToken*) v_b;
	return a->nonce - b->nonce;
}

