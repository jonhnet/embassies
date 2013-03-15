#include "LinuxKvmCanvasAcceptor.h"

LinuxKvmCanvasAcceptor::LinuxKvmCanvasAcceptor(App *app, LongMessageAllocator *lmaalloc)
	: app(app),
	  lmaalloc(lmaalloc),
	  lma(NULL),
	  framebuffer(NULL),
	  framebuffer_size(0)
{}

ZCanvas *LinuxKvmCanvasAcceptor::make_canvas(uint32_t framebuffer_size)
{
	this->framebuffer_size = framebuffer_size;
	lma = lmaalloc->allocate(framebuffer_size);
	framebuffer = (uint8_t*) lma->map();
	assert(framebuffer!=NULL);

	// TODO LEAK we need to associate this lma with the xbc, so that when
	// the xbc is torn down, we can lma->unlink() and reclaim the shm
	// segment.

	return &canvas;
}

uint8_t *LinuxKvmCanvasAcceptor::get_framebuffer()
{
	return framebuffer;
}

uint32_t LinuxKvmCanvasAcceptor::get_framebuffer_size()
{
	return framebuffer_size;
}

ZCanvas *LinuxKvmCanvasAcceptor::get_canvas()
{
	if (lma!=NULL)
	{
		return &canvas;
	}
	else
	{
		return NULL;
	}
}
