#include <base/printf.h>

#include "CanvasDownsampler.h"

CanvasDownsampler::CanvasDownsampler(ZCanvas *canvas, uint8_t *np_bits, MallocFactory *mf)
	: canvas_id(canvas->canvas_id),
	  canvas(canvas)
{
	lite_assert(canvas->pixel_format == zoog_pixel_format_truecolor24);
	lite_assert(sizeof(Pixel_zoog_true24)==sizeof(uint32_t));
	this->mf = mf;
	uint32_t app_canvas_size =
		sizeof(Pixel_zoog_true24) * canvas->width * canvas->height;
	this->app_canvas = (Pixel_zoog_true24 *) mf_malloc(mf, app_canvas_size);
//	np_canvas = new Chunky_canvas<Pixel_rgb565> canvas((Pixel_rgb565 *)np_bits, Area(canvas->width, canvas->height));
	uint32_t np_canvas_size = sizeof(Pixel_rgb565) * canvas->width * canvas->height;
	this->np_canvas = (Pixel_rgb565 *) np_bits;
	PDBG("np canvas @%p--%p (%x bytes0; app_canvas @%p--%p (%x bytes)",
		np_canvas, ((uint8_t*) np_canvas)+np_canvas_size, np_canvas_size,
		app_canvas, ((uint8_t*) app_canvas)+app_canvas_size, app_canvas_size);
	canvas->data = (uint8_t*) app_canvas;
}

CanvasDownsampler::CanvasDownsampler(ZCanvasID canvas_id)
	: canvas_id(canvas_id),
	  canvas(NULL)
{
}

CanvasDownsampler::~CanvasDownsampler()
{
	// NB as of writing, there isn't actually a path to get here yet
	if (canvas != NULL)
	{
		mf_free(mf, app_canvas);
	}
}

void CanvasDownsampler::update(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height)
{
	lite_assert(x0+width <= canvas->width);
	lite_assert(y0+height <= canvas->height);

	// TODO undoubtedly the slowest, silliest way to blit these pixels.
	// The ideal solution would be to plumb a 32-bpp Pixel through NitPicker.
	uint32_t canvas_width = canvas->width;
	uint32_t y1=y0+height, x1=x0+width;
	uint32_t xi, yi;
	for (yi=y0; yi<y1; yi++)
	{
		uint32_t row_start = yi*canvas_width;
		Pixel_zoog_true24 *app_row = &app_canvas[row_start];
		Pixel_rgb565 *np_row = &np_canvas[row_start];
		
		for (xi=x0; xi<x1; xi++)
		{
			Pixel_zoog_true24 zp = app_row[xi];
			np_row[xi] = Pixel_rgb565(zp.r(), zp.g(), zp.b());
		}
	}
	int idx = y0*canvas_width+x0;
//	PDBG("first pixel (%d,%d) idx %d %06x (z) %04x (np)",
//		x0, y0, idx, app_canvas[idx], np_canvas[idx]);
}

uint32_t CanvasDownsampler::hash(const void *v_a)
{
	CanvasDownsampler *a = (CanvasDownsampler *) v_a;
	return a->canvas_id;
}

int CanvasDownsampler::cmp(const void *v_a, const void *v_b)
{
	CanvasDownsampler *a = (CanvasDownsampler *) v_a;
	CanvasDownsampler *b = (CanvasDownsampler *) v_b;
	return a->canvas_id - b->canvas_id;
}

//////////////////////////////////////////////////////////////////////////////

CanvasDownsamplerManager::CanvasDownsamplerManager(MallocFactory *mf)
	: mf(mf)
{
	hash_table_init(&ht, mf, &CanvasDownsampler::hash, &CanvasDownsampler::cmp);
}

CanvasDownsamplerManager::~CanvasDownsamplerManager()
{
	lite_assert(ht.count==0);	// todo clean up after myself
	hash_table_free(&ht);
}

void CanvasDownsamplerManager::create(ZCanvas *canvas, uint8_t *np_bits)
{
	CanvasDownsampler *cd = new CanvasDownsampler(canvas, np_bits, mf);
	hash_table_insert(&ht, cd);
}

CanvasDownsampler *CanvasDownsamplerManager::lookup(ZCanvasID canvas_id)
{
	CanvasDownsampler key(canvas_id);
	CanvasDownsampler *cd = (CanvasDownsampler *) hash_table_lookup(&ht, &key);
	return cd;
}
