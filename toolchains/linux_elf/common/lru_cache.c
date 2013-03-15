#include <stdio.h>	// NULL
#include "LiteLib.h"
#include "lru_cache.h"

void lrucache_init(LRUCache *lru, MallocFactory *mf, int max_size, lru_free_f *lru_free, CmpFunc cmp_func)
{
	linked_list_init(&lru->ll, mf);
	lru->max_size = max_size;
	lru->lru_free = lru_free;
	lru->cmp_func = cmp_func;
}

void _lrucache_reduce(LRUCache *lru, int max_count)
{
	while (lru->ll.count > max_count)
	{
		void *datum = linked_list_remove_tail(&lru->ll);
		(lru->lru_free)(datum);
	}
}

void lrucache_free(LRUCache *lru)
{
	_lrucache_reduce(lru, 0);
}

void *lrucache_find(LRUCache *lru, void *key)
{
	LinkedListLink *link;
	void *datum = NULL;
	for (link = lru->ll.head; link!=NULL; link=link->next)
	{
		if (lru->cmp_func(key, link->datum)==0)
		{
			datum = link->datum;
			linked_list_remove_middle(&lru->ll, link);
			linked_list_insert_head(&lru->ll, datum);
			break;
		}
	}
	return datum;
}

void lrucache_insert(LRUCache *lru, void *item)
{
	void *existing_datum = lrucache_find(lru, item);
	lite_assert(existing_datum==NULL);
	linked_list_insert_head(&lru->ll, item);
	_lrucache_reduce(lru, lru->max_size);
}

