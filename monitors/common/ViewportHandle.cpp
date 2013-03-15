#include "ViewportHandle.h"

ViewportHandle::ViewportHandle(
	ViewportID handle,
	uint32_t process_id,
	BlitterViewport *viewport)
	: handle(handle), process_id(process_id), viewport(viewport)
{
}

BlitterViewport *ViewportHandle::get_viewport()
{
	return viewport;
}

ViewportID ViewportHandle::get_viewport_id()
{
	return handle;
}

uint32_t ViewportHandle::hash(const void *v_a)
{
	ViewportHandle *a = (ViewportHandle *) v_a;
	return a->handle ^ (a->process_id<<16 | a->process_id>>16);
}

int ViewportHandle::cmp(const void *v_a, const void *v_b)
{
	ViewportHandle *a = (ViewportHandle *) v_a;
	ViewportHandle *b = (ViewportHandle *) v_b;
	int c1 = (a->process_id - b->process_id);
	if (c1!=0) { return c1; }
	int c2 = (a->handle - b->handle);
	return c2;
}

