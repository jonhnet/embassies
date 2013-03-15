#include "GenodeCanvasAcceptor.h"

GenodeCanvasAcceptor::GenodeCanvasAcceptor(ZoogMonitor::Session::AcceptCanvasReply *acr)
	: acr(acr)
{}

void GenodeCanvasAcceptor::set_fb_dataspace(Genode::Dataspace_capability dcap)
{
	acr->framebuffer_dataspace_cap = dcap;
}

ZCanvas *GenodeCanvasAcceptor::get_zoog_canvas()
{
	return &acr->canvas;
}
