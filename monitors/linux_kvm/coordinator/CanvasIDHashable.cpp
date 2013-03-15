#include <malloc.h>
#include <string.h>
#include "CanvasIDHashable.h"
#include "hash_table.h"

CanvasIDHashable::CanvasIDHashable(ZCanvasID canvas_id)
{
	this->canvas_id = canvas_id;
}

uint32_t CanvasIDHashable::hash()
{
	return canvas_id;
}

int CanvasIDHashable::cmp(Hashable *v_other)
{
	CanvasIDHashable *other = (CanvasIDHashable *) v_other;
	return canvas_id - other->canvas_id;
}
