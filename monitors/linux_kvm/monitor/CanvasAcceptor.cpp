#include <assert.h>
#include "ZoogVM.h"
#include "CanvasAcceptor.h"

CanvasAcceptor::CanvasAcceptor(
	ZoogVM *vm,
	ViewportID viewport_id,
	PixelFormat *known_formats,
	int num_formats,
	ZCanvas *out_canvas)
{
	this->vm = vm;
	this->viewport_id = viewport_id;
	assert(num_formats>0 && num_formats <= 4);
	this->num_formats = num_formats;
	memcpy(this->known_formats, known_formats, sizeof(known_formats[0])*num_formats);
	this->out_canvas = out_canvas;
	this->event = vm->get_sync_factory()->new_event(false);
	this->slot = NULL;
}

CanvasAcceptor::CanvasAcceptor(ViewportID viewport_id)
{
	this->vm = NULL;
	this->viewport_id = viewport_id;
	this->out_canvas = NULL;
	this->event = NULL;
	this->slot = NULL;
}

CanvasAcceptor::~CanvasAcceptor()
{
	if (this->event!=NULL)
	{
		delete this->event;
	}
}

void *CanvasAcceptor::allocate_memory_for_incoming_canvas(uint32_t capacity)
{
	// this mmaps empty memory into place, which we then overwrite,
	// but that's probably okay, because the empty memory opn is probably
	// pretty cheap anyway.
	slot = vm->allocate_guest_memory(capacity, "canvas");
	return slot->get_host_addr();
}

void CanvasAcceptor::signal_canvas_ready(ZCanvas *received_canvas)
{
	lite_assert(slot!=NULL);	// hey, you never told me where the canvas was!
	memcpy(out_canvas, received_canvas, sizeof(ZCanvas));
	out_canvas->data = (uint8_t*) slot->get_guest_addr();
	event->signal();
}

uint32_t CanvasAcceptor::hash(const void *v_a)
{
	return ((CanvasAcceptor*)v_a)->viewport_id;
}

int CanvasAcceptor::cmp(const void *v_a, const void *v_b)
{
	return ((CanvasAcceptor*)v_a)->viewport_id
			- ((CanvasAcceptor*)v_a)->viewport_id;
}

void CanvasAcceptor::wait()
{
	event->wait();
}

