#include "zoogcpp.h"

#include "pal_abi/pal_extensions.h"
#include "xax_extensions.h"
#include "perf_measure.h"
#include "ZRectangle.h"
#include "vendor-a-chain.h"
#include "ambient_malloc.h"
#include "zoog_malloc_factory.h"
#include "misc.h"

ZoogGraphics zg;

void zoog_init()
{
	zg.zdt = xe_get_dispatch_table();
	zg.painted = false;

	ZoogMallocFactory zmf;
	zoog_malloc_factory_init(&zmf, zg.zdt);
	ambient_malloc_init(zmf_get_mf(&zmf));

	zoog_verify_label(zg.zdt);
	
	debug_create_toplevel_window_f* dctw = (debug_create_toplevel_window_f*)
		(zg.zdt->zoog_lookup_extension)(DEBUG_CREATE_TOPLEVEL_WINDOW_NAME);
	zg.viewport = (dctw)();
	
	PixelFormat pf[1] = {zoog_pixel_format_truecolor24};
	(zg.zdt->zoog_map_canvas)(zg.viewport, pf, 1, &zg.canvas);

	zg.palette[BLACK]		= 0x000000;
	zg.palette[DKGREY]		= 0x555555;
	zg.palette[GREY]		= 0xaaaaaa;
	zg.palette[WHITE]		= 0xffffff;
	zg.palette[DKRED]		= 0x800000;
	zg.palette[RED]			= 0xff0000;
	zg.palette[DKGREEN]		= 0x008000;
	zg.palette[GREEN]		= 0x00ff00;
	zg.palette[DKBLUE]		= 0x000080;
	zg.palette[BLUE]		= 0x0000ff;
	zg.palette[DKYELLOW]	= 0x808000;
	zg.palette[YELLOW]		= 0xffff00;
	zg.palette[DKCYAN]		= 0x008080;
	zg.palette[CYAN]		= 0x00ffff;
	zg.palette[DKMAGENTA]	= 0x800080;
	zg.palette[MAGENTA]		= 0xff00ff;
}

void zoog_verify_label(ZoogDispatchTable_v1 *zdt)
{
	ZCertChain *zcc = new ZCertChain((uint8_t*) vendor_a_chain, sizeof(vendor_a_chain));
	(zdt->zoog_verify_label)(zcc);
}

void zoog_paint_display()
{
	ZRectangle zr = NewZRectangle(0, 0, zg.canvas.width, zg.canvas.height);
	(zg.zdt->zoog_update_canvas)(zg.canvas.canvas_id, &zr);

	if (!zg.painted)
	{
		PerfMeasure pm(zg.zdt);
		pm.mark_time("first_paint");
		zg.painted = true;
	}
}

#define CANVAS_PIXEL(x,y) ((uint32_t*) (zg.canvas.data+(y*zg.canvas.width+x)*4))

int zoog_drawpixel(int x, int y)
{
	*CANVAS_PIXEL(x,y) = zg.palette[zg.cur_color];
	return 0;
}

void zoog_setcolor(int color)
{
	zg.cur_color = color;
	// see misc.h for color palette table.
}

int zoog_drawscansegment(unsigned char *colors, int x0, int y, int length)
{
	for (int xd=0; xd<length; xd++)
	{
		int x = x0+xd;
		*CANVAS_PIXEL(x,y) = zg.palette[colors[xd]];
	}
	return 0;
}


