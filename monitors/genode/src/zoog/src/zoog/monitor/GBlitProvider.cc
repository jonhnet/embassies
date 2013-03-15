#include "GBlitProvider.h"
#include "GBlitProviderCanvas.h"

GBlitProvider::GBlitProvider(
	Timer::Connection *timer)
	: timer(timer)
{
}

BlitProviderCanvasIfc *GBlitProvider::create_canvas(
	ZCanvasID canvas_id,
	WindowRect *wr,
	BlitCanvasAcceptorIfc *canvas_acceptor,
	ProviderEventDeliveryIfc *event_delivery_ifc,
	BlitProviderCanvasIfc *parent)
{
	GCanvasAcceptorIfc *gacceptor = (GCanvasAcceptorIfc *) canvas_acceptor;
	GBlitProviderCanvas *gparent = (GBlitProviderCanvas *) parent;

	GBlitProviderCanvas *gcanvas = new GBlitProviderCanvas(
		this,
		canvas_id,
		wr,
		gacceptor,
		event_delivery_ifc,
		gparent,
		timer);

	return gcanvas;
}
