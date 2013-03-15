#include "LiteLib.h"
#include "TinyPPM.h"
#include "math_util.h"

TinyPPM::TinyPPM(char *buf)
{
	data = (uint8_t*) parse_header(buf);
}

const char* TinyPPM::read_line()
{
	const char *this_line;
	while (true)
	{
		this_line = hdr_line;
		hdr_line = lite_index(this_line, '\n')+1;
		if (this_line[0]!='#')
		{
			break;
		}
	}
	return this_line;
}

char* TinyPPM::parse_header(char *buf)
{
	hdr_line = buf;

	const char *type = read_line();
	lite_assert(lite_memcmp(type, "P6\n", 3)==0);
	const char *dims = read_line();
		const char *space = lite_index(dims, ' ');
		lite_assert(space!=NULL);
		width = lite_atoi(dims);
		height = lite_atoi(space+1);
	const char *depth = read_line();
	lite_assert(lite_memcmp(depth, "255\n", 4)==0);
	return hdr_line;
}

void TinyPPM::paint_on(ZCanvas *zcanvas)
{
#define CANVAS_PIXEL ((uint32_t*) (zcanvas->data+(y*zcanvas->width+x)*4))

	uint32_t background = 0x00a06000;
	uint32_t minx = min(zcanvas->width, width);
	uint32_t miny = min(zcanvas->height, height);
	uint32_t x, y;
	for (y=0; y<miny; y++)
	{
		for (x=0; x<minx; x++)
		{
			uint8_t *ptr = data+(y*width + x)*3;
			*CANVAS_PIXEL = ptr[0]<<16 | ptr[1]<<8 | ptr[0];
		}
		for (; x<zcanvas->width; x++)
		{
			*CANVAS_PIXEL = background;
		}
	}
	for (; y<zcanvas->height; y++)
	{
		for (x=0; x<zcanvas->width; x++)
		{
			*CANVAS_PIXEL = background;
		}
	}
}
