#include "LiteLib.h"
#include "ZutexQueue.h"

ZutexQueue::ZutexQueue(MallocFactory *mf, uint32_t guest_hdl)
{
	linked_list_init(&queue, mf);
	this->guest_hdl = guest_hdl;
	this->is_key = false;
}

ZutexQueue::ZutexQueue(uint32_t guest_hdl)
{
	this->guest_hdl = guest_hdl;
	this->is_key = true;
}

ZutexQueue::~ZutexQueue()
{
	if (!is_key)
	{
		lite_assert(is_empty());
	}
}

void ZutexQueue::push(ZutexWaiter *zw)
{
	linked_list_insert_tail(&queue, zw);
}

ZutexWaiter *ZutexQueue::pop()
{
	return (ZutexWaiter*) linked_list_remove_head(&queue);
}

void ZutexQueue::remove(ZutexWaiter *zw)
{
	LinkedListIterator lli;
	for (ll_start(&queue, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		if (((ZutexWaiter*) ll_read(&lli)) == zw)
		{
			ll_remove(&queue, &lli);
			return;
		}
	}
	lite_assert(false);	// expected ZutexWaiter not present.
}

bool ZutexQueue::is_empty()
{
	return queue.count==0;
}

uint32_t ZutexQueue::hash(const void *datum)
{
	ZutexQueue *z_a = (ZutexQueue *) datum;
	return z_a->guest_hdl;
}

int ZutexQueue::cmp(const void *a, const void *b)
{
	ZutexQueue *z_a = (ZutexQueue *) a;
	ZutexQueue *z_b = (ZutexQueue *) b;
	return z_a->guest_hdl - z_b->guest_hdl;
}

