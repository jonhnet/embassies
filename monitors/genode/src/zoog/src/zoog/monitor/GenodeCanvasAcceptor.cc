#include "GenodeCanvasAcceptor.h"

GenodeCanvasAcceptor::GenodeCanvasAcceptor(ZoogMonitor::Session::MapCanvasReply *mcr)
	: mcr(mcr)
{}

void GenodeCanvasAcceptor::set_fb_dataspace(Genode::Dataspace_capability dcap)
{
	mcr->framebuffer_dataspace_cap = dcap;
}

ZCanvas *GenodeCanvasAcceptor::get_zoog_canvas()
{
	return &mcr->canvas;
}
