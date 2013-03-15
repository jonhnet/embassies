#include "DeedEntry.h"

DeedEntry::DeedEntry(Deed deed, BlitterViewport *viewport)
	: deed(deed), viewport(viewport)
{
}

BlitterViewport *DeedEntry::get_viewport()
{
	return viewport;
}

uint32_t DeedEntry::hash(const void *v_a)
{
	DeedEntry* a = (DeedEntry*) v_a;
	return (uint32_t) a->deed;
}

int DeedEntry::cmp(const void *v_a, const void *v_b)
{
	DeedEntry* a = (DeedEntry*) v_a;
	DeedEntry* b = (DeedEntry*) v_b;
	return (int) (a->deed - b->deed);
}
