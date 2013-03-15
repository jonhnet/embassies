#include "cheesylock.h"

#include "LiteLib.h"

#define GUARD0 0x0fa731fe
#define GUARD1 0xef137af0

static uint32_t get_parent_frame_ptr(void)
{
	uint32_t parent_frame_ptr;
	__asm__(
		"movl	(%%ebp), %%eax;"
		"movl	%%eax, %0;"
		: "=r"(parent_frame_ptr)
		:
		: "eax");
	return parent_frame_ptr;
}

void cheesy_lock_init(CheesyLock *cl)
{
	cl->guard0 = GUARD0;
	cl->value = 0;
	cl->debug_frame_ptr = 0;
	cl->guard1 = GUARD1;
}

// http://lists.canonical.org/pipermail/kragen-tol/1999-August/000457.html
void cheesy_lock_acquire(CheesyLock *cl)
{
	uint32_t value;

	lite_assert(cl->guard0==GUARD0 && cl->guard1==GUARD1);

	while (1)
	{
		value = 1;
		__asm__ volatile (
			"xchg %0, (%2);"
			: "=a"(value)
			: "0"(value), "r"(&cl->value)
			);
		if (value==0)
			break;
		lite_assert(value==1);
	}
	lite_assert(cl->debug_frame_ptr == 0);
	cl->debug_frame_ptr = get_parent_frame_ptr();
}

void cheesy_lock_release(CheesyLock *cl)
{
	uint32_t pebp = get_parent_frame_ptr();
	if (cl->guard0!=GUARD0
		|| cl->guard1!=GUARD1
		|| cl->debug_frame_ptr != pebp)
	{
		lite_assert(0);
	}
	cl->debug_frame_ptr = 0;
	cl->value = 0;
}
