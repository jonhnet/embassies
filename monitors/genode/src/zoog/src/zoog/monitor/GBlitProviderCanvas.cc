#include <base/env.h>
#include <input/event.h>

#include "GBlitProviderCanvas.h"
#include "GBlitProvider.h"

GBlitProviderCanvas::GBlitProviderCanvas(
	GBlitProvider *gblit_provider,
	ZCanvasID canvas_id,
	ZRectangle *wr,
	const char* window_label,
	GCanvasAcceptorIfc *gacceptor,
	ProviderEventDeliveryIfc *event_delivery_ifc,
	GBlitProviderCanvas *parent_gbc,
	Timer::Connection *timer)
	: timer(timer),
	  input_handler_thread(this),
	  event_delivery_ifc(event_delivery_ifc)
{
	this->canvas_id = canvas_id;
	open_nitpicker_window(wr, window_label);
	gacceptor->set_fb_dataspace(nitpicker->framebuffer()->dataspace());

	ZCanvas *canvas = gacceptor->get_zoog_canvas();
	canvas->canvas_id = canvas_id;
	canvas->pixel_format = zoog_pixel_format_truecolor24;
	canvas->width = wr->width;
	canvas->height = wr->height;
	canvas->data = (uint8_t*) 0x00f00f00;

	wpos = *wr;

	input_handler_thread.start();
}

GBlitProviderCanvas::~GBlitProviderCanvas()
{
}

void GBlitProviderCanvas::open_nitpicker_window(ZRectangle *wr, const char* window_label)
{
	nitpicker = new Nitpicker::Connection(wr->width, wr->height);
	_cap = nitpicker->create_view();
	Nitpicker::View_client vc = Nitpicker::View_client(_cap);
	vc.title(window_label);
	vc.viewport(wr->x, wr->y, wr->width, wr->height, 0, 0, true);
	vc.stack(Nitpicker::View_capability(), true, true);

// This is what the *PAL* does.
//	void *addr = env()->rm_session()->attach(
//		nitpicker->framebuffer()->dataspace());
//	Chunky_canvas<Pixel_rgb565> canvas((Pixel_rgb565 *)addr, Area(fb_w, fb_h));
}

ZCanvasID GBlitProviderCanvas::get_canvas_id()
{
	return canvas_id;
}

void GBlitProviderCanvas::update_image(ZRectangle *wr)
{
	nitpicker->framebuffer()->refresh(
		wpos.x+wr->x, wpos.y+wr->y, wr->width, wr->height);
}

void GBlitProviderCanvas::reconfigure(ZRectangle *wr)
{
	PDBG("unimpl");
	lite_assert(false);
}

uint32_t GBlitProviderCanvas::hash()
{
	return canvas_id;
}

int GBlitProviderCanvas::cmp(Hashable *other)
{
	return canvas_id - ((GBlitProviderCanvas *) other)->canvas_id;
}

void GBlitProviderCanvas::run_input_handler_thread()
{
	using namespace Genode;

	PDBG("RUnning!");

	Input::Event *ev_buf = env()->rm_session()->attach(
		nitpicker->input()->dataspace());

	while (1) {
		PDBG("loop top");

		while (!nitpicker->input()->is_pending()) {
//			nitpicker->framebuffer()->refresh(0, 0, win_w, win_h);
			timer->msleep(200);
		}

		for (int i = 0, num_ev = nitpicker->input()->flush(); i < num_ev; i++)
		{
			Input::Event *ev = &ev_buf[i];

			// Translate nitpicker events into Zoog events.
			ZoogUIEvent zevt;
			if (ev->type() == Input::Event::PRESS
				|| ev->type() == Input::Event::RELEASE)
			{
				zevt.type = (ev->type() == Input::Event::PRESS)
					? zuie_key_down : zuie_key_up;
				zevt.un.key_event.canvas_id = canvas_id;
				int zoog_key = key_translate.map_nitpicker_key_to_zoog(ev->keycode());
				if (zoog_key == key_translate.INVALID)
				{
					PDBG("Untranslated key %d", ev->keycode());
					continue;
				}
				zevt.un.key_event.keysym = zoog_key;
			}
			else if (ev->type() == Input::Event::MOTION)
			{
				zevt.type = zuie_mouse_move;
				zevt.un.mouse_event.canvas_id = canvas_id;
				zevt.un.mouse_event.x = ev->ax();
				zevt.un.mouse_event.y = ev->ay();
			}
#if 0
			else if (ev->type() == Input::Event::LEAVE)
			{
				zevt.type == zuie_blur;
				zevt.un.canvas_event.canvas_id = canvas_id;
				// TODO synthesize focus events when any other
				// event type arrives after a LEAVE.
			}
#endif
			else
			{
				// ignore;
				continue;
			}

			event_delivery_ifc->deliver_ui_event(&zevt);
		}
	}
}

void GBlitProviderCanvas::move(int x, int y)
{
	wpos.x = x, wpos.y = y;
	Nitpicker::View_client(_cap).viewport(wpos.x, wpos.y, wpos.width, wpos.height, 0, 0, true);
}

void GBlitProviderCanvas::top()
{
	Nitpicker::View_client(_cap).stack(Nitpicker::View_capability(), true, true);
}

