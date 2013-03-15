#include "pal_abi/pal_ui.h"

#include "paltest.h"
#include "uitest.h"
#include "LiteLib.h"
#include "cheesy_snprintf.h"
#include "ZRectangle.h"

void blocking_receive_ui_event(ZoogDispatchTable_v1 *zdt, ZoogHostAlarms *alarms, ZoogUIEvent *evt)
{
	while (1)
	{
		uint32_t match_val = alarms->receive_ui_event;
		(zdt->zoog_receive_ui_event)(evt);
		if (evt->type != zuie_no_event) { break; }
		ZutexWaitSpec spec = { &alarms->receive_ui_event, match_val };
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
}

void stripes(ZCanvas *canvas, uint32_t color1, uint32_t color2)
{
	uint32_t y, x;
	uint32_t x_stride = zoog_bytes_per_pixel(canvas->pixel_format);
	uint32_t y_stride = x_stride * canvas->width;

	for (y=0; y<canvas->height; y++)
	{
		for (x=0; x<canvas->width; x++)
		{
			uint32_t *pixel = (uint32_t*) &canvas->data[y*y_stride+x*x_stride];
			pixel[0] = ((x+(y>>3)) & 1) ? color1 : color2;
		}
	}
}

void uitest(ZoogDispatchTable_v1 *zdt)
{
	ZoogHostAlarms *alarms = (zdt->zoog_get_alarms)();

#if 0

	// wait for my delegation event; should be the first one anyway,
	// since all the others involve Viewports.
	while (1)
	{
		// Just block; not yet testing zutex.
		blocking_receive_ui_event(zdt, alarms, &evt);
		if (evt.type == zuie_viewport_created)
		{
			break;
		}
		(g_debuglog)("stderr", "(uitest) dropping UI event\n");
	}
#endif

	debug_create_toplevel_window_f* debug_create_toplevel_window =
		(debug_create_toplevel_window_f*)
		(zdt->zoog_lookup_extension)(DEBUG_CREATE_TOPLEVEL_WINDOW_NAME);
	ViewportID viewport = debug_create_toplevel_window();

	PixelFormat known_formats[] = { zoog_pixel_format_truecolor24 };
	ZCanvas canvas;
	(zdt->zoog_map_canvas)(
		viewport,
		known_formats,
		1, // num_formats
		&canvas);
	lite_assert(canvas.data!=NULL);

	ZRectangle zr;
	stripes(&canvas, 0x00ff0000, 0x008f8fff);
	{
		zr = NewZRectangle(0, 0, canvas.width, canvas.height);
		(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);
	}

	stripes(&canvas, 0x0000ff00, 0x0000ff00);
	{
		zr = NewZRectangle(canvas.width*1/4, canvas.height*1/4, canvas.width*1/4, canvas.height*1/4);
		(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);
	}

	stripes(&canvas, 0x000000ff, 0x000000ff);
	{
		zr = NewZRectangle(canvas.width*2/4, canvas.height*2/4, canvas.width*1/4, canvas.height*1/4);
		(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);
	}

	stripes(&canvas, 0x000000ff, 0x008000ff);
	{
		zr = NewZRectangle(canvas.width*3/8, canvas.height*1/8, canvas.width*1/4, canvas.height*3/4);
		(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);
	}

	stripes(&canvas, 0x000000ff, 0x00008040);
	{
		zr = NewZRectangle(canvas.width*1/8, canvas.height*3/8, canvas.width*3/4, canvas.height*1/4);
		(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);
	}

	ZoogUIEvent evt;
	while (1)
	{
		char msg[1024];
		blocking_receive_ui_event(zdt, alarms, &evt);

		if (evt.type == zuie_key_down)
		{
			cheesy_snprintf(msg, sizeof(msg), "down sym %d\n",
				evt.un.key_event.keysym);
		}
		else if (evt.type == zuie_key_up)
		{
			cheesy_snprintf(msg, sizeof(msg), "up sym %d\n",
				evt.un.key_event.keysym);
		}
		else if (evt.type == zuie_mouse_move)
		{
			cheesy_snprintf(msg, sizeof(msg), "mouse loc %d,%d\n",
				evt.un.mouse_event.x, evt.un.mouse_event.y);
		}
		else
		{
			cheesy_snprintf(msg, sizeof(msg), "unhandled type %x\n", evt.type);
		}
		(g_debuglog)("stderr", msg);
	}
}
