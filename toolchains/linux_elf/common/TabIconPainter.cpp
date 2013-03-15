#include <stdio.h>
#include <malloc.h>

#include "TabIconPainter.h"
#include "AppIdentity.h"
#include "ZRectangle.h"

#ifndef ZOOG_ROOT
#include "zoog_root.h"
#endif

void TabIconPainter::load(const char *icon_path)
{
#define FAIL(m)	{ fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, (m)); goto fail; }

	rows = 0;
	cols = 0;
	row_pointers = NULL;
	FILE *fp = fopen(icon_path, "rb");
	if (fp==NULL) { FAIL("failed to open file"); }

	png_byte header[8];
	fread(header, 1, sizeof(header), fp);
	if (png_sig_cmp(header, 0, 8)!=0) { FAIL("bogus png hdr"); }

	// zarb.org/~gc/html/libpng.html
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) { FAIL("fail"); }
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr ) { FAIL("fail"); }

	// insane setjmp-based error handling
	if (setjmp(png_jmpbuf(png_ptr))) { FAIL("png init_io error"); }

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	cols = png_get_image_width(png_ptr, info_ptr);
	rows = png_get_image_height(png_ptr, info_ptr);
	
	if (setjmp(png_jmpbuf(png_ptr))) { FAIL("png read error"); }
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * rows);
	for (int y=0; y<rows; y++)
	{
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
	}
	png_read_image(png_ptr, row_pointers);
fail:
	fclose(fp);
}

TabIconPainter::TabIconPainter()
{
	AppIdentity ai;
	char path[1000];
	snprintf(path, sizeof(path), ZOOG_ROOT "/toolchains/linux_elf/crypto/favicons/%s.png", ai.get_identity_name());
	load(path);
}

TabIconPainter::TabIconPainter(const char *icon_path)
{
	load(icon_path);
}

TabIconPainter::~TabIconPainter()
{
	if (row_pointers!=NULL)
	{
		for (int y=0; y<rows; y++)
		{
			free(row_pointers[y]);
		}
		free(row_pointers);
	}
}

void TabIconPainter::paint_canvas(ZCanvas *zcanvas)
{
#define CANVAS_PIXEL ((uint32_t*) (zcanvas->data+(y*zcanvas->width+x)*4))
	for (uint32_t y=0; y<zcanvas->height; y++)
	{
		for (uint32_t x=0; x<zcanvas->width; x++)
		{
			*CANVAS_PIXEL = 0x00ffffff;
		}
	}
			
	if (row_pointers!=NULL)
	{
		for (uint32_t y=0; y<zcanvas->height && y<(uint32_t) rows; y++)
		{
			for (uint32_t x=0; x<zcanvas->width && x<(uint32_t) cols; x++)
			{
				uint8_t *in_pixel = (uint8_t*) (&row_pointers[y][x*3]);
				*CANVAS_PIXEL = in_pixel[0]<<16 | in_pixel[1]<<8 | in_pixel[2];
//				pixel pp = ppmdata[y][x];
//				*CANVAS_PIXEL = PPM_GETR(pp)<<16 | PPM_GETG(pp)<<8 | PPM_GETB(pp);
			}
		}
	}
}

