#include "LinuxDbgCanvasAcceptor.h"

LinuxDbgCanvasAcceptor::LinuxDbgCanvasAcceptor(Process *process, XpReplyMapCanvas *reply)
	: process(process),
	  reply(reply),
	  framebuffer_size(0),
	  framebuffer(NULL)
{}

ZCanvas *LinuxDbgCanvasAcceptor::make_canvas(uint32_t framebuffer_size)
{
	XaxPortBuffer *xpb = process->get_buf_pool()->get_buffer(framebuffer_size, reply);
	ZCanvas *canvas = &xpb->un.canvas.canvas;
	this->framebuffer_size = framebuffer_size;
	this->framebuffer = (uint8_t*) &canvas[1];
	return canvas;
}

uint8_t *LinuxDbgCanvasAcceptor::get_framebuffer()
{
	return framebuffer;
}

uint32_t LinuxDbgCanvasAcceptor::get_framebuffer_size()
{
	return framebuffer_size;
}
