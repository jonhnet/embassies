#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "relocate_this.h"
#include "ZCertChain.h"
#include "ambient_malloc.h"
#include "zoog_malloc_factory.h"
#include "TinyPPM.h"
#include "ZRectangle.h"

#include "vendor-a-chain.h"
#include "splash_image.h"
#include "perf_measure.h"

extern "C" {
void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count);
}

void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count)
{
	relocate_this(elfbase, rel_offset, rel_count);

	PerfMeasure pm(zdt);
	pm.mark_time("start");

	ZoogMallocFactory zmf;
	zoog_malloc_factory_init(&zmf, zdt);
	ambient_malloc_init(zmf_get_mf(&zmf));

	debug_create_toplevel_window_f* dctw = (debug_create_toplevel_window_f*)
		(zdt->zoog_lookup_extension)(DEBUG_CREATE_TOPLEVEL_WINDOW_NAME);

//	pm.mark_time("a");
	ZCertChain *zcc = new ZCertChain((uint8_t*) vendor_a_chain, sizeof(vendor_a_chain));
	(zdt->zoog_verify_label)(zcc);
//	pm.mark_time("verify");

	ViewportID viewport = (dctw)();

	PixelFormat pf[1] = {zoog_pixel_format_truecolor24};
	ZCanvas canvas;
	(zdt->zoog_map_canvas)(viewport, pf, 1, &canvas);
	
//	pm.mark_time("g");
	TinyPPM ppm(splash_image);
//	pm.mark_time("parse_splash");
	ppm.paint_on(&canvas);
//	pm.mark_time("paint");
	ZRectangle zr = NewZRectangle(0, 0, canvas.width, canvas.height);
	(zdt->zoog_update_canvas)(canvas.canvas_id, &zr);

	pm.mark_time("done");

	uint32_t stall_zutex = 0;
	ZutexWaitSpec spec = { &stall_zutex, 0 };
	while (true)
	{
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
}
