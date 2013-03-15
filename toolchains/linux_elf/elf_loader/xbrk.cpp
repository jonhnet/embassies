#include <errno.h>	// ENOMEM

#include "LiteLib.h"
#include "xbrk.h"
#include "xax_util.h"
#include "cheesy_snprintf.h"

void Xbrk::init(ZoogDispatchTable_v1 *zdt, AbstractMmapIfc *ami, MallocTraceFuncs *mtf)
{
	this->zdt = zdt;
	this->ami = ami;
	this->mtf = mtf;
	this->capacity = NUM_RANGES;
	this->count = 0;
	this->next_new_brk = 0;
}

void Xbrk::allow_new_brk(uint32_t size, const char *dbg_desc)
{
	xax_assert(count < capacity);
	XbrkRange *xr = &ranges[count];
	count += 1;

	xr->start = (uint8_t*) (ami->mmap_f)(ami, size);
	xr->current = xr->start;
	xr->end = xr->start + size;
	xr->dbg_desc = dbg_desc;
}

int Xbrk::wait_until_all_brks_claimed()
{
	ZutexWaitSpec spec;
	while (1)
	{
		spec.zutex = &next_new_brk;
		spec.match_val = next_new_brk;
		if (spec.match_val == count)
		{
			break;
		}
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
	// This return value shows up in debug_strace,
	// but the xe_ call has void return type -- it's not meant to
	// be visible / used by apps.
	return spec.match_val;
}

void *Xbrk::new_brk()
{
	xax_assert(next_new_brk < count);	// no brk(0) waiting for this guy.
	XbrkRange *xr = &ranges[next_new_brk];
	next_new_brk+=1;
	// someone has claimed a brk. Signal wait_until_all...()
	(zdt->zoog_zutex_wake)(
		&next_new_brk,
		next_new_brk,
		ZUTEX_ALL,
		0,
		NULL,
		0);
	return xr->start;
}

void *Xbrk::more_brk(void *new_location_v)
{
	uint8_t *new_location = (uint8_t*) new_location_v;
	// linear search. Okay because we expect count to be very small,
	// and brk()s not all that common anyway.
	uint32_t i;
	for (i=0; i<count; i++)
	{
		XbrkRange *xr = &ranges[i];
		if (xr->start <= new_location
			&& new_location <= xr->end)
		{
			// Turns out malloc.c's sYSTRIm really is willing to
			// give back brk, so we don't assert that brk only grows.
			uint32_t len = new_location - xr->current;
			xi_trace_alloc(mtf, tl_brk, xr->current, len);
			xr->current = new_location;
			return new_location;
		}
	}
	xax_assert(0); // "brk() past end of budgeted VM allocation")
	return (void*) -ENOMEM;
}

void Xbrk::debug_dump_allocations()
{
	uint32_t i;
	for (i=0; i<count; i++)
	{
		XbrkRange *xr = &ranges[i];

		char buf[500];
		char hexbuf[16];
		
		lite_strcpy(buf, "used ");
		lite_strcat(buf, hexstr(hexbuf, xr->current - xr->start));
		lite_strcat(buf, " of ");
		lite_strcat(buf, hexstr(hexbuf, xr->end - xr->start));
		lite_strcat(buf, ": ");
		lite_strcat(buf, xr->dbg_desc);
		lite_strcat(buf, "\n");
		debug_logfile_append(zdt, "stderr", buf);
	}
}
