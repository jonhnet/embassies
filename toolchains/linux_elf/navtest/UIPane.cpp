#include <stdlib.h>
#include <string.h>

#include "NavTest.h"
#include "UIPane.h"
#include "NavigationProtocol.h"
#include "ZRectangle.h"
#include "TabIconPainter.h"
#include "UrlEP.h"

const int num_colors = 6;
const uint32_t colorpixel[num_colors] =
	{ 0xff0000, 0xff8000, 0xffff00, 0x00ff00, 0x0000ff, 0x8000ff };

void UIPane::_paint_canvas(ZCanvas *zcanvas)
{
#define PIXEL ((uint32_t*)(zcanvas->data+(4*(y*zcanvas->width+x))))
	for (uint32_t y=0; y<zcanvas->height; y++)
	{
		for (uint32_t x=0; x<zcanvas->width; x++)
		{
			*PIXEL = colorpixel[_color];
		}
	}
	for (uint32_t y=5; y<10; y++)
	{
		for (uint32_t dot=0; dot<_color; dot++)
		{
			for (uint32_t x=dot*10; x<dot*10+5; x++)
			{
				*PIXEL = 0;
			}
		}
	}
	ZRectangle zr = NewZRectangle(0, 0, zcanvas->width, zcanvas->height);
	_zdt()->zoog_update_canvas(zcanvas->canvas_id, &zr);
}

ZoogDispatchTable_v1* UIPane::_zdt()
{
	return nav_test->get_zdt();
}

UIPane::UIPane(NavTest* nav_test)
	: nav_test(nav_test),
	  callout_ifc(NULL)
{
	nav_test->add_pane(this);
}

bool UIPane::has_canvas(ZCanvasID canvas_id)
{
	return (_pane_canvas.canvas_id == canvas_id);
}

void UIPane::setup_callout_ifc(NavCalloutIfc *callout_ifc)
{
	this->callout_ifc = callout_ifc;
}

void UIPane::navigate(EntryPointIfc *entry_point)
{
	UrlEP* url_ep = (UrlEP*) entry_point;
	_color = atoi(url_ep->get_url()) % num_colors;
}

void UIPane::no_longer_used()
{
	nav_test->remove_pane(this);
	delete this;
}

void UIPane::pane_revealed(ViewportID pane_tenant_id)
{
	PixelFormat pixel_format = zoog_pixel_format_truecolor24;
	_zdt()->zoog_map_canvas(
		pane_tenant_id, &pixel_format, 1, &_pane_canvas);
	fprintf(stderr, "UIPane revealed; mapping pane canvas %d (this %p)\n",
		_pane_canvas.canvas_id, this);

	_paint_canvas(&_pane_canvas);
}

void UIPane::pane_hidden()
{
	_zdt()->zoog_unmap_canvas(_pane_canvas.canvas_id);
}

void UIPane::forward()
{
	int new_color = (_color + 1) % num_colors;
	char buf[200];
	snprintf(buf, sizeof(buf), "%d.html", new_color);

	AppIdentity ai;
	UrlEP *ep = new UrlEP(buf);
	callout_ifc->transfer_forward(ai.get_identity_name(), ep);
}

void UIPane::back()
{
	callout_ifc->transfer_backward();
}

void UIPane::new_tab()
{
	lite_assert(false);	// unimpl
}

