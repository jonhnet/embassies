#include "pal_abi/pal_ui.h"

#include "paltest.h"
#include "uitest.h"
#include "nesttest.h"
#include "LiteLib.h"
#include "vendor_a_pub_key.h"
#include "ZRectangle.h"

class Nesttest {
private:
	ZoogDispatchTable_v1 *zdt;
	ZoogHostAlarms *alarms;

	void solid(ZCanvas *canvas, uint32_t color);
	ViewportID get_toplevel();
	void map_canvas(ViewportID viewport, ZCanvas *out_canvas, uint32_t color);
	Deed sublet(ViewportID parent_viewport, ZCanvas *parent_canvas);

public:
	Nesttest(ZoogDispatchTable_v1 *zdt);
};

ViewportID Nesttest::get_toplevel()
{
	debug_create_toplevel_window_f* debug_create_toplevel_window =
		(debug_create_toplevel_window_f*)
		(zdt->zoog_lookup_extension)(DEBUG_CREATE_TOPLEVEL_WINDOW_NAME);
	return debug_create_toplevel_window();
}

void Nesttest::map_canvas(ViewportID viewport, ZCanvas *out_canvas, uint32_t color)
{
	PixelFormat known_formats[] = { zoog_pixel_format_truecolor24 };
	(zdt->zoog_map_canvas)(viewport, known_formats, 1, out_canvas);
	lite_assert(out_canvas->data!=NULL);

	solid(out_canvas, color);
	ZRectangle zr = NewZRectangle(0, 0, out_canvas->width, out_canvas->height);
	(zdt->zoog_update_canvas)(out_canvas->canvas_id, &zr);
}

Deed Nesttest::sublet(ViewportID parent_viewport, ZCanvas *parent_canvas)
{
	ZRectangle sublet_region = NewZRectangle(
		parent_canvas->width*1/8,
		parent_canvas->height*3/8,
		parent_canvas->width*3/4,
		parent_canvas->height*1/4);
	ViewportID landlord_viewport;
	Deed sublet_deed;
	(zdt->zoog_sublet_viewport)(parent_viewport, &sublet_region, &landlord_viewport, &sublet_deed);
	return sublet_deed;
}

Nesttest::Nesttest(ZoogDispatchTable_v1 *zdt)
{
	this->zdt = zdt;
	alarms = (zdt->zoog_get_alarms)();

	ViewportID top_viewport = get_toplevel();
	ZCanvas top_canvas;
	map_canvas(top_viewport, &top_canvas, 0x00ff0000);

	Deed child_deed = sublet(top_viewport, &top_canvas);

	ViewportID child_viewport;
	DeedKey deed_key;
	(zdt->zoog_accept_viewport)(&child_deed, &child_viewport, &deed_key);
	ZCanvas child_canvas;
	map_canvas(child_viewport, &child_canvas, 0x000040ff);
}

void Nesttest::solid(ZCanvas *canvas, uint32_t color)
{
	uint32_t y, x;
	uint32_t x_stride = zoog_bytes_per_pixel(canvas->pixel_format);
	uint32_t y_stride = x_stride * canvas->width;

	for (y=0; y<canvas->height; y++)
	{
		for (x=0; x<canvas->width; x++)
		{
			uint32_t *pixel = (uint32_t*) &canvas->data[y*y_stride+x*x_stride];
			pixel[0] = color;
		}
	}
}

void nesttest(ZoogDispatchTable_v1 *zdt)
{
	Nesttest nt(zdt);
}
