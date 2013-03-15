#include <stdio.h>
#include "AckWait.h"

AckWait::AckWait(uint64_t rpc_nonce, SyncFactory *sf)
  : rpc_nonce(rpc_nonce),
    evt(sf->new_event(false))
{ }

AckWait::AckWait(uint64_t rpc_nonce)
  : rpc_nonce(rpc_nonce),
    evt(NULL)
{ }

AckWait::~AckWait()
{
	delete(evt);
}

bool AckWait::received(int timeout_msec)
{
	return evt->wait(timeout_msec);
}

void AckWait::signal()
{
	evt->signal();
}

uint32_t AckWait::hash(const void *v_a)
{
	AckWait* a = (AckWait*) v_a;
	return (uint32_t) ((a->rpc_nonce>>32) | a->rpc_nonce);
}

int AckWait::cmp(const void *v_a, const void *v_b)
{
	AckWait* a = (AckWait*) v_a;
	AckWait* b = (AckWait*) v_b;
	int64_t cmp = a->rpc_nonce - b->rpc_nonce;
	return cmp>0 ? 1 : (cmp<0 ? -1 : 0);
}
