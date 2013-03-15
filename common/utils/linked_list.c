#include "LiteLib.h"
#include "linked_list.h"

void linked_list_init(LinkedList *ll, MallocFactory *mf)
{
	ll->mf = mf;
	ll->head = NULL;
	ll->tail = NULL;
	ll->count = 0;
}

void linked_list_insert_head(LinkedList *ll, void *datum)
{
	LinkedListLink *link = (LinkedListLink *)
		mf_malloc(ll->mf, sizeof(LinkedListLink));
	link->datum = datum;
	link->next = ll->head;
	link->prev = NULL;
	ll->head = link;
	if (link->next == NULL)
	{
		ll->tail = link;
	}
	else
	{
		link->next->prev = link;
	}
	ll->count += 1;
}

void linked_list_insert_tail(LinkedList *ll, void *datum)
{
	LinkedListLink *link = (LinkedListLink *)
		mf_malloc(ll->mf, sizeof(LinkedListLink));
	link->datum = datum;
	link->prev = ll->tail;
	link->next = NULL;
	ll->tail = link;
	if (link->prev == NULL)
	{
		ll->head = link;
	}
	else
	{
		link->prev->next = link;
	}
	ll->count += 1;
}

void *linked_list_remove_head(LinkedList *ll)
{
	LinkedListLink *link = ll->head;
	if (link==NULL)
	{
		lite_assert(ll->tail==NULL);
		return NULL;
	}
	ll->head = link->next;
	if (ll->head==NULL)
	{
		lite_assert(ll->tail == link);
		ll->tail = NULL;
	}
	else
	{
		ll->head->prev = NULL;
	}
	void *datum = link->datum;
	mf_free(ll->mf, link);
	ll->count -= 1;
	return datum;
}

void *linked_list_remove_tail(LinkedList *ll)
{
	LinkedListLink *link = ll->tail;
	if (link==NULL)
	{
		lite_assert(ll->head==NULL);
		return NULL;
	}
	ll->tail = link->prev;
	if (ll->tail==NULL)
	{
		lite_assert(ll->head == link);
		ll->head = NULL;
	}
	else
	{
		ll->tail->next = NULL;
	}
	void *datum = link->datum;
	mf_free(ll->mf, link);
	ll->count -= 1;
	return datum;
}

void linked_list_remove(LinkedList *ll, void *datum)
{
	LinkedListIterator lli;
	for (ll_start(ll, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		if (ll_read(&lli)==datum)
		{
			linked_list_remove_middle(ll, lli);
			return;
		}
	}
	lite_assert(false); // expected element not found
}

void linked_list_remove_middle(LinkedList *ll, LinkedListLink *link)
{
	if (ll->head==link)
	{
		lite_assert(link->prev==NULL);
		ll->head = link->next;
	}
	else
	{
		link->prev->next = link->next;
	}

	if (ll->tail==link)
	{
		lite_assert(link->next==NULL);
		ll->tail = link->prev;
	}
	else
	{
		link->next->prev = link->prev;
	}
	mf_free(ll->mf, link);
	ll->count -= 1;
}

LinkedListLink *linked_list_find(LinkedList *ll, void *datum)
{
	LinkedListLink *link;
	for (link = ll->head; link!=NULL; link=link->next)
	{
		if (link->datum == datum)
		{
			return link;
		}
	}
	return NULL;
}

void linked_list_visit_all(LinkedList *ll, visit_f *visit, void *user_data)
{
	LinkedListLink *link;
	for (link = ll->head; link!=NULL; link=link->next)
	{
		bool carry_on = (visit)(link->datum, user_data);
		if (!carry_on)
		{
			break;
		}
	}
}
