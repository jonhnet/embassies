#include <assert.h>
#include "ZoogVM.h"
#include "ViewportDelegator.h"

uint32_t ViewportDelegator::next_rpc_nonce = 3;

ViewportDelegator::ViewportDelegator(
	ZoogVM *vm,
	xa_delegate_viewport *xa,
	xa_delegate_viewport *xa_guest)
{
	this->rpc_nonce = next_rpc_nonce++;		// TODO race
	this->xa = xa;
	this->out_viewport_id = -1;
	this->event = vm->get_sync_factory()->new_event(false);
	this->pub_key = xa_guest->in_pub_key;
}

ViewportDelegator::ViewportDelegator(uint32_t rpc_nonce)
{
	this->rpc_nonce = rpc_nonce;
	this->event = NULL;
}

ViewportDelegator::~ViewportDelegator()
{
	if (this->event!=NULL)
	{
		delete this->event;
	}
}

ZCanvasID ViewportDelegator::get_canvas_id()
{
	return xa->in_canvas_id;
}

uint32_t ViewportDelegator::get_pub_key_len()
{
	return xa->in_pub_key_len;
}

uint8_t *ViewportDelegator::get_pub_key()
{
	// provides a pointer to guest-visible data;
	// the assumption is that the caller is about to copy it out
	// to host-safe memory. Note that len is in host-safe memory,
	// so our length-validity check can't get raced.
	return pub_key;
}

void ViewportDelegator::signal_reply_ready(ViewportID viewport_id)
{
	this->out_viewport_id = viewport_id;
	event->signal();
}

uint32_t ViewportDelegator::hash(const void *v_a)
{
	ViewportDelegator *a = (ViewportDelegator *) v_a;
	return a->rpc_nonce;
}

int ViewportDelegator::cmp(const void *v_a, const void *v_b)
{
	ViewportDelegator *a = (ViewportDelegator *) v_a;
	ViewportDelegator *b = (ViewportDelegator *) v_b;
	return a->rpc_nonce - b->rpc_nonce;
}

ViewportID ViewportDelegator::wait()
{
	event->wait();
	return this->out_viewport_id;
}


