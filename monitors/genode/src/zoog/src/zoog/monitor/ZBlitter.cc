/*
 * \brief  Zoog Blitter -- mapping from zoog blit ui to nitpicker blit impl
 * \author Jon Howell
 * \date   2012-03-11
 */

#include <base/printf.h>
#include <base/env.h>
#include <input/event.h>

#include "LiteAssert.h"
#include "ZBlitter.h"

ZBlitter::ZBlitter()
	: input_handler_thread(this)
{
	win_w = 500;
	win_h = 400;
	_x = 15;
	_y = 40;

	Color fg(255,0,0);
	Color bg(0,64,128);
	const char *title = "zblitter";

	using namespace Genode;

	PDBG("ZBlitter()");
	int fb_w=1024, fb_h = 768;

	nitpicker = new Nitpicker::Connection(fb_w, fb_h);

	_cap = nitpicker->create_view();
	//PDBG("_cap is tid %08x local_name %08x\n", _cap.dst(), _cap.local_name());
	Nitpicker::View_client(_cap).title(title);
	Nitpicker::View_client(_cap).viewport(_x, _y, win_w, win_h, 0, 0, true);
	Nitpicker::View_client(_cap).stack(Nitpicker::View_capability(), true, true);
	
	void *addr = env()->rm_session()->attach(
		nitpicker->framebuffer()->dataspace());
	Chunky_canvas<Pixel_rgb565> canvas((Pixel_rgb565 *)addr, Area(fb_w, fb_h));

	canvas.draw_box(Rect(Point(10, 10), Area(win_w-20, win_h-20)), bg);
	int x;
	for (x=0; x<200; x+=10)
	{
//		PDBG("red rect at %d", x);
		canvas.draw_box(Rect(Point(x, x), Area(10, 10)), fg);
	}
	PDBG("ZBlitter() pokes refresh at nitpicker");
	nitpicker->framebuffer()->refresh(_x, _y, win_w, win_h);
	PDBG("ZBlitter() done");

	timer = new Timer::Connection();
	input_handler_thread.start();
}

void ZBlitter::move(int x, int y)
{
	_x = x, _y = y;
	Nitpicker::View_client(_cap).viewport(_x, _y, win_w, win_h, 0, 0, true);
}

void ZBlitter::top()
{
	Nitpicker::View_client(_cap).stack(Nitpicker::View_capability(), true, true);
}

void ZBlitter::run_input_handler_thread()
{
	using namespace Genode;

	PDBG("RUnning!");

	Input::Event *ev_buf = env()->rm_session()->attach(
		nitpicker->input()->dataspace());
	int omx = 0, omy = 0, key_cnt = 0;
	while (1) {
		PDBG("loop top");

		while (!nitpicker->input()->is_pending()) {
			nitpicker->framebuffer()->refresh(0, 0, win_w, win_h);
			timer->msleep(20);
		}

		for (int i = 0, num_ev = nitpicker->input()->flush(); i < num_ev; i++)
		{
			Input::Event *ev = &ev_buf[i];

			if (ev->type() == Input::Event::PRESS)   key_cnt++;
			if (ev->type() == Input::Event::RELEASE) key_cnt--;

			/* move view */
			if (ev->type() == Input::Event::MOTION && key_cnt > 0)
			{
				PDBG("omx %d ax %d omy %d ay %d",
					omx, ev->ax(), omy, ev->ay());
				int newx = _x + ev->ax() - omx;
				int newy = _y + ev->ay() - omy;
				PDBG("move view from %d,%d to %d,%d", _x, _y, newx, newy);
				move(newx, newy);
			}

			/* find selected view and bring it to front */
			if (ev->type() == Input::Event::PRESS && key_cnt == 1)
			{
				PDBG("top");
				top();
			}

			omx = ev->ax(); omy = ev->ay();
		}
	}
}
