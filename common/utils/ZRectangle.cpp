#include "ZRectangle.h"

ZRectangle NewZRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	ZRectangle zr = {x, y, width, height };
	return zr;
}
