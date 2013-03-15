#include "ZutexWaiter.h"
#include "ZoogVCPU.h"

ZutexWaiter::ZutexWaiter(MallocFactory *mf, SyncFactory *sf, ZoogVCPU *dbg_vcpu)
{
	linked_list_init(&waiting_on_queues, mf);
	event = sf->new_event(false);
	this->dbg_vcpu = dbg_vcpu;
}

void ZutexWaiter::wait(VCPUYieldControlIfc *vyci)
{
	if (vyci->vcpu_held())
	{
		bool finished = event->wait(vyci->vcpu_yield_timeout_ms());
		if (finished)
		{
			// fprintf(stderr, "ZutexWaiter::wait Finished fast\n");
			return;
		}
		vyci->vcpu_yield();
	}
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

int ZutexWaiter::dbg_get_vcpu_zid()
{
	return dbg_vcpu->get_zid();
}
