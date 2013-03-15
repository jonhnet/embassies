#include "ZutexWaiter.h"

ZutexWaiter::ZutexWaiter(MallocFactory *mf, SyncFactory *sf)
{
	linked_list_init(&waiting_on_queues, mf);
	event = sf->new_event(true);
}

void ZutexWaiter::wait()
{
	event->wait();
}

void ZutexWaiter::wake()
{
	event->signal();
}

void ZutexWaiter::reset_event()
{
	event->reset();
}

void ZutexWaiter::append_list(ZutexQueue *zq)
{
	linked_list_insert_tail(&waiting_on_queues, zq);
}

LinkedList *ZutexWaiter::get_zq_list()
{
	return &waiting_on_queues;
}
