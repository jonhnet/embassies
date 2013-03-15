#include "BlitterViewport.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "BlitterCanvas.h"
#include "BlitterManager.h"

uint32_t BlitterViewport::next_viewport_id = 1;

BlitterViewport::BlitterViewport(
	BlitterManager *blitter_mgr,
	uint32_t process_id,
	BlitterViewport *parent_viewport,
	ZRectangle *rectangle)
	: window_configuration(*rectangle)
{
	this->blitter_mgr = blitter_mgr;
	this->landlord_handle = new ViewportHandle(next_viewport_id++, process_id, this);
	this->tenant_handle = NULL;
	this->parent_viewport = parent_viewport;
	this->mapped_canvas = NULL;

	blitter_mgr->add_viewport_handle(this->landlord_handle);
	blitter_mgr->manufacture_deed(this, &deed, &deed_key);

#if 0
	ZoogUIEvent evt;
	evt.type = zuie_viewport_created;
	evt.un.viewport_event.viewport_id = viewport_id;
	blitter_mgr->multicast_ui_event(vendor, &evt);
	// TODO SAFETY this code should *make a copy of* vendor (since caller
	// owns it), and stash a copy, to enable monitor to ensure that the
	// process that comes to collect the canvas indeed belongs to the
	// appropriate vendor. (Until we fix this, there's an easy
	// "guessing attack" a malicious app could use to steal viewports.)
#endif
}

#if 0
BlitterViewport::BlitterViewport(ViewportID viewport_id)
{
	this->viewport_id = viewport_id;
	this->mapped_canvas = NULL;
}
#endif

BlitterViewport::~BlitterViewport()
{
	if (mapped_canvas!=NULL)
	{
		mapped_canvas->unmap();
	}
	lite_assert(false);	// unimpl : should recurse and close tenants
	if (tenant_handle!=NULL)
	{
		blitter_mgr->remove_viewport_handle(this->tenant_handle);
		delete this->tenant_handle;
	}
	if (deed!=0)
	{
		lite_assert(false);	// next line unimpl
		// blitter_mgr->remove_deed(deed);
	}
	blitter_mgr->remove_viewport_handle(this->landlord_handle);
	delete this->landlord_handle;
}

void BlitterViewport::map_canvas(
	BlitPalIfc *palifc,
	PixelFormat *known_formats,
	int num_formats,
	BlitCanvasAcceptorIfc *acceptor,
	const char *window_label)
{
	lite_assert(mapped_canvas == NULL);
	mapped_canvas = new BlitterCanvas(
		blitter_mgr,
		palifc,
		this,
		known_formats,
		num_formats,
		acceptor,
		window_label);
	blitter_mgr->add_canvas(mapped_canvas);
}

#if 0
// we actually probably want a call like this to reconfigure
// sublets without destroying an entire tree (and waiting for a tree
// of applications to put them back together), but it's unplumped right now.
void BlitterViewport::update_viewport(ZRectangle *wr)
{
	window_configuration = *wr;

	if (mapped_canvas!=NULL)
	{
		mapped_canvas->update_viewport(&window_configuration);
	}
}
#endif

ViewportID BlitterViewport::get_landlord_id()
{
	return landlord_handle->get_viewport_id();
}

Deed BlitterViewport::get_deed()
{
	return deed;
}

DeedKey BlitterViewport::get_deed_key()
{
	return deed_key;
}

ViewportID BlitterViewport::get_tenant_id()
{
	lite_assert(tenant_handle!=NULL);
	return tenant_handle->get_viewport_id();
}

BlitProviderCanvasIfc *BlitterViewport::get_parent_canvas()
{
	if (parent_viewport==NULL)
	{
		// I'm a toplevel viewport, with no canvas (and hence xbc) above me.
		return NULL;
	}
	return parent_viewport->mapped_canvas->get_pc();
}

#if 0
uint32_t BlitterViewport::hash(const void *v_a)
{
	BlitterViewport *a = (BlitterViewport *) v_a;
	return a->viewport_id;
}

int BlitterViewport::cmp(const void *v_a, const void *v_b)
{
	BlitterViewport *a = (BlitterViewport *) v_a;
	BlitterViewport *b = (BlitterViewport *) v_b;
	return a->viewport_id - b->viewport_id;
}
#endif

void BlitterViewport::canvas_closed(BlitterCanvas *canvas)
{
	lite_assert(this->mapped_canvas == canvas);
	this->mapped_canvas = NULL;
}

Deed BlitterViewport::transfer()
{
	if (this->mapped_canvas!=NULL)
	{
		mapped_canvas->unmap();
	}
	lite_assert(this->tenant_handle!=NULL);
	blitter_mgr->remove_viewport_handle(this->tenant_handle);
	delete this->tenant_handle;
	this->tenant_handle = NULL;
	blitter_mgr->manufacture_deed(this, &deed, &deed_key);
	return deed;
}

void BlitterViewport::accept_viewport(uint32_t process_id)
{
	this->tenant_handle = new ViewportHandle(next_viewport_id++, process_id, this);
	this->deed = 0;
	blitter_mgr->add_viewport_handle(this->tenant_handle);
}

BlitterViewport* BlitterViewport::sublet(ZRectangle *rectangle, uint32_t process_id)
{
	BlitterViewport* child = new BlitterViewport(blitter_mgr, process_id, this, rectangle);
	return child;
}
