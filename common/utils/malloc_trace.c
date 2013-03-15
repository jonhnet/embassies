#include <stdio.h>	// NULL
#include "malloc_trace.h"

void xi_trace_alloc(MallocTraceFuncs *mtf, TraceLabel label, void *addr, uint32_t size)
{
	if (mtf != NULL)
	{
		(mtf->tr_alloc)(label, addr, size);
	}
}

void xi_trace_free(MallocTraceFuncs *mtf, TraceLabel label, void *addr)
{
	if (mtf != NULL)
	{
		(mtf->tr_free)(label, addr);
	}
}


