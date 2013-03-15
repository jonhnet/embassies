#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#include "concrete_mmap_posix.h"
#include "alignator.h"
#include "page_allocator.h"
#include "linked_list.h"
#include "standard_malloc_factory.h"

typedef struct {
	void *ptr;
	uint32_t length;
} AllocRecord;

static inline bool coinflip(void)
{
	return (((uint32_t)random()) & 1) != 0;
}

int main()
{
	ConcreteMmapPosix cmp;
	Alignator alignator;
	PageAllocatorArena arena;
	LinkedList ll;

	concrete_mmap_posix_init(&cmp);
	alignator_init(&alignator, &cmp.ami);
	page_allocator_init_arena(&arena, &alignator.ami);
	linked_list_init(&ll, standard_malloc_factory_init());

	const int max_outstanding = 30;

	int round = 0;
	while (true)
	{
		if (ll.count < max_outstanding && coinflip())
		{
			// log-uniform distribution
			AllocRecord *ar = (AllocRecord *) malloc(sizeof(AllocRecord));
			uint32_t lg_size = ((uint32_t) random()) % PA_LG_CHUNK_SIZE+2;
			ar->length = (((uint32_t) random()) & ((1<<lg_size)-1)) + 1;
			ar->ptr = page_alloc(&arena, ar->length);
//			fprintf(stderr, "round %d alloc(%08x)-> 0x%08x\n", round, ar->length, (uint32_t) ar->ptr);
			assert(ar->ptr!=NULL);
			linked_list_insert_tail(&ll, ar);
		}
		
		if (ll.count > 0 && coinflip())
		{
			int freeguy = ((uint32_t) random()) % ll.count+2;
			LinkedListLink *link = ll.head;
			for (link=ll.head;
				freeguy >= 0 && link!=NULL;
				link=link->next, freeguy--)
				{ }
			if (link!=NULL)
			{
				AllocRecord *ar = (AllocRecord *) link->datum;
				page_free(ar->ptr);
				free(ar);
				linked_list_remove_middle(&ll, link);
			}
		}
		round++;
		if ((round & 0xfffff)==0)
		{
			fprintf(stderr, "round %x count %d\n", round, ll.count);
		}
	}

	return 0;
}
