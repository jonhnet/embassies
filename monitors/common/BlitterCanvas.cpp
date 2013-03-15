#include "BlitterCanvas.h"
#include "BlitterManager.h"
#include "BlitterViewport.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"

BlitterCanvas::BlitterCanvas(
	BlitterManager *blitter_mgr,
	BlitPalIfc *palifc,
	BlitterViewport *viewport,
	PixelFormat *known_formats,
	int num_formats,
	BlitCanvasAcceptorIfc *acceptor,
	const char *window_label)
{
	this->blitter_mgr = blitter_mgr;
	this->palifc = palifc;
	this->viewport = viewport;
	this->provider = blitter_mgr->get_provider();
	this->uidelivery = palifc->get_uidelivery();

	int i;
	for (i=0; i<num_formats; i++)
	{
		if (known_formats[i]==zoog_pixel_format_truecolor24)
		{
			break;
		}
	}
	if (i==num_formats)
	{
		lite_assert(false);	// no matching pixel format! That's surprising.
		//acceptor->no_canvas_ready();
		return;
	}

	ZRectangle *wr = viewport->get_window_configuration();

	ZCanvasID canvas_id = blitter_mgr->allocate_canvas_id();
	pc = provider->create_canvas(canvas_id, wr, acceptor, this, viewport->get_parent_canvas(), window_label);

	this->canvas_id = pc->get_canvas_id();
}

BlitterCanvas::BlitterCanvas(ZCanvasID canvas_id)
	: pc(NULL),
	  canvas_id(canvas_id)
{ }

BlitterCanvas::~BlitterCanvas()
{
	delete pc;
}

void BlitterCanvas::unmap()
{
	viewport->canvas_closed(this);
		// releases pointer to this
	blitter_mgr->remove_canvas(this);
		// releases (hashtable's) pointer to this
	delete this;
}

void BlitterCanvas::update_canvas(ZRectangle *wr)
{
	if (pc==NULL)
	{
		palifc->debug_remark("sorry, dude, that window already closed. Check your UI event queue.\n");
		return;
	}
	pc->update_image(wr);
}

#if 0
void BlitterCanvas::update_viewport(ZRectangle *wr)
{
	pc->reconfigure(wr);
}
#endif

uint32_t BlitterCanvas::hash(const void *v_a)
{
	BlitterCanvas *a = (BlitterCanvas *) v_a;
	return a->canvas_id;
}

int BlitterCanvas::cmp(const void *v_a, const void *v_b)
{
	BlitterCanvas *a = (BlitterCanvas *) v_a;
	BlitterCanvas *b = (BlitterCanvas *) v_b;
	return a->canvas_id - b->canvas_id;
}

void BlitterCanvas::notify_canvas_closed(BlitProviderCanvasIfc *pc)
{
	lite_assert(pc == this->pc);
	this->pc = NULL;
}

void BlitterCanvas::deliver_ui_event(ZoogUIEvent *zuie)
{
	uidelivery->deliver_ui_event(zuie);
}
