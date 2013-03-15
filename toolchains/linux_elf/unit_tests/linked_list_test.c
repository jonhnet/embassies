#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "LiteLib.h"
#include "linked_list.h"
#include "standard_malloc_factory.h"

const int range = 1000;

void print_list(LinkedList *ll)
{
	LinkedListLink *link;
	fprintf(stderr, "[");
	for (link = ll->head; link!=NULL; link=link->next)
	{
		fprintf(stderr, "%d%s",
			((int*)link->datum)[0],
			link->next!=NULL ? ", " : "");

		if (link->prev!=NULL)
		{
			lite_assert(link->prev->next == link);
		}
		if (link->next!=NULL)
		{
			lite_assert(link->next->prev == link);
		}
	}
	fprintf(stderr, "]\n");
}

void do_remove_test()
{
	LinkedList ll;
	linked_list_init(&ll, standard_malloc_factory_init());

	int i;
	int data[10];
	for (i=0; i<10; i++)
	{
		data[i] = i;
		linked_list_insert_head(&ll, &data[i]);
	}

	for (i=0; i<10; i+=2)
	{
		LinkedListLink *match = linked_list_find(&ll, &data[i]);
		linked_list_remove_middle(&ll, match);
	}

	fprintf(stderr, "odd list: ");
	LinkedListLink *link;
	for (link = ll.head; link!=NULL; link=link->next)
	{
		fprintf(stderr, "%d ", ((int*) link->datum)[0]);
	}
	fprintf(stderr, "\n");

	link = ll.head;
	for (i=9; i>=1; i-=2)
	{
		lite_assert(link->datum == (void*) &data[i]);
		link = link->next;
	}
}

int main()
{
	do_remove_test();

	LinkedList ll;
	int data[range];

	linked_list_init(&ll, standard_malloc_factory_init());

	int i;
	for (i=0; i<range; i++)
	{
		data[i] = i;
	}

	int v_head = 500;
	int v_tail = 499;

	int round = 0;
	while (1)
	{
		int what = ((uint32_t) random()) % 4;
		switch (what)
		{
			case 0:
				v_head = (v_head-1+range)%range;
#if DEBUG
				fprintf(stderr, "insert head %d\n", v_head);
#endif
				linked_list_insert_head(&ll, &data[v_head]);
				if (ll.count==1)
				{
					v_tail = v_head;
				}
				break;
			case 1:
				v_tail = (v_tail+1+range)%range;
#if DEBUG
				fprintf(stderr, "insert tail %d\n", v_tail);
#endif
				linked_list_insert_tail(&ll, &data[v_tail]);
				if (ll.count==1)
				{
					v_head = v_tail;
				}
				break;
			case 2:
				if (ll.count>0)
				{
					void *datum = linked_list_remove_head(&ll);
					int val = ((int*)datum)[0];
#if DEBUG
					fprintf(stderr, "remove head %d\n", val);
#endif
					lite_assert(val==v_head);
					v_head = (v_head+1+range)%range;
				}
				break;
			case 3:
				if (ll.count>0)
				{
					void *datum = linked_list_remove_tail(&ll);
					int val = ((int*)datum)[0];
#if DEBUG
					fprintf(stderr, "remove tail %d\n", val);
#endif
					lite_assert(val==v_tail);
					v_tail = (v_tail-1+range)%range;
				}
				break;
			default:
				lite_assert(0);
		}

#if DEBUG
		print_list(&ll);
		fprintf(stderr, "head %d tail %d\n\n", v_head, v_tail);
#endif

		if ((round&0xfffff)==0)
		{
			fprintf(stderr, "round %d count %d\n", round, ll.count);
		}
		round++;
	}
	
	return 0;
}
