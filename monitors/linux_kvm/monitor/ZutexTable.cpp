#include "safety_check.h"
#include "ZutexTable.h"
#include "LiteLib.h"

ZutexTable::ZutexTable(MallocFactory *mf, SyncFactory *sf, MemoryMapIfc *memory_map)
{
	this->mf = mf;
	hash_table_init(&table, mf, ZutexQueue::hash, ZutexQueue::cmp);
	mutex = sf->new_mutex(false);
	this->memory_map = memory_map;
#if DEBUG_DISASTER
	dbg_sanity("ZutexTable ctor");
#endif //DEBUG_DISASTER
}

void ZutexTable::_lock()
{
	mutex->lock();
}

void ZutexTable::_unlock()
{
	mutex->unlock();
}

ZutexQueue *ZutexTable::_get_queue(uint32_t guest_hdl)
{
	ZutexQueue key(guest_hdl);
	return (ZutexQueue *) hash_table_lookup(&table, &key);
}

ZutexQueue *ZutexTable::_get_or_create_queue(uint32_t guest_hdl)
{
	ZutexQueue *zq = _get_queue(guest_hdl);
	if (zq!=NULL)
	{
		return zq;
	}
	zq = new ZutexQueue(mf, guest_hdl);
	hash_table_insert(&table, zq);
	return zq;
}

void ZutexTable::_free_zq(void *v_this, void *v_zq)
{
	ZutexTable *ztable = (ZutexTable *) v_this;
	ZutexQueue *zq = (ZutexQueue *) v_zq;
	lite_assert(zq->is_empty());
#if DEBUG_DISASTER
	fprintf(stderr, "_free(ZQ@%08x)\n", zq->dbg_get_guest_hdl());
#endif //DEBUG_DISASTER
	hash_table_remove(&ztable->table, zq);
	delete zq;
#if DEBUG_DISASTER
	ztable->dbg_sanity("after _free");
#endif //DEBUG_DISASTER
}

void ZutexTable::wait(ZutexWaitSpec *specs, uint32_t count, ZutexWaiter *waiter, VCPUYieldControlIfc *vyci)
{
	uint32_t i;
	bool all_match = true;

	_lock();
#if DEBUG_DISASTER
	dbg_sanity("wait");
#endif //DEBUG_DISASTER

	// check match vals. Protected by zutex_table lock, so if any change
	// after we check them, we know their corresponding zutex_wakes
	// won't occur until we unlock, so they'll see our Waiter in the
	// ZutexQueue and still wake us up. The change won't get lost.
	memory_map->lock_memory_map();
		// We could lock/unlock around eac map_region/deref pair,
		// but eh, that's just more lockin'. Contention schmontention.
	for (i=0; i<count; i++)
	{
		uint32_t guest_zutex = (uint32_t) specs[i].zutex;
		uint32_t *host_zutex =
			(uint32_t*) memory_map->map_region_to_host(guest_zutex, sizeof(uint32_t));
		// TODO this host read will have to be aware of no-read perms
		// if we ever support them, since those perms will have the same
		// effect here in my address space. Such a change should
		// change the api to map_region_to_host, so it should get detected
		// by the compiler.

		SAFETY_CHECK(host_zutex!=NULL);	// guest zutex not actually mapped!

		uint32_t current_guest_val = *host_zutex;
		if (current_guest_val != specs[i].match_val)
		{
			all_match = false;
			break;
		}
	}
	memory_map->unlock_memory_map();
	
	if (!all_match)
	{
		_unlock();
		return;
	}
	for (i=0; i<count; i++)
	{
		uint32_t guest_zutex = (uint32_t) specs[i].zutex;
		ZutexQueue *zq = _get_or_create_queue((uint32_t) guest_zutex);
		zq->push(waiter);
		waiter->append_list(zq);
	}
	waiter->reset_event();
	_unlock();

	// Go to sleep. Here the thread sits until someone finds our Waiter
	// and wakes us up.
	waiter->wait(vyci);

	// NB we unlocked the table before we wait()ed, so don't fall through
	// to do it again.
	return;

fail:
	memory_map->unlock_memory_map();
	_unlock();
}

void ZutexTable::_enumerate(void *v_ll, void *v_zq)
{
	LinkedList *zqs = (LinkedList *) v_ll;
	ZutexQueue *zq = (ZutexQueue *) v_zq;
	linked_list_insert_tail(zqs, zq);
}

void ZutexTable::dbg_dump_waiter(ZutexWaiter *zw)
{
	fprintf(stderr, "    W(#%d): ", zw->dbg_get_vcpu_zid());

	LinkedList *qs = zw->get_zq_list();
	LinkedListIterator llv;
	for (ll_start(qs, &llv); ll_has_more(&llv); ll_advance(&llv))
	{
		ZutexQueue *zv = (ZutexQueue *) ll_read(&llv);
		fprintf(stderr, "ZQ(@%08x) ", zv->dbg_get_guest_hdl());
	}
	fprintf(stderr, "\n");
}

void ZutexTable::dbg_sanity(const char *desc)
{
	LinkedList zqs;
	linked_list_init(&zqs, mf);
	hash_table_visit_every(&table, _enumerate, &zqs);

	fprintf(stderr, "ZutexTable(at %s)\n", desc);
	LinkedListIterator llq;
	for (ll_start(&zqs, &llq); ll_has_more(&llq); ll_remove(&zqs, &llq))
	{
		ZutexQueue *zq = (ZutexQueue *) ll_read(&llq);
		fprintf(stderr, "  ZQ(@%08x)\n", zq->dbg_get_guest_hdl());

		LinkedList *ws = zq->dbg_get_waiters();
		LinkedListIterator llw;
		for (ll_start(ws, &llw); ll_has_more(&llw); ll_advance(&llw))
		{
			ZutexWaiter *zw = (ZutexWaiter *) ll_read(&llw);
			dbg_dump_waiter(zw);
		}
	}
}

uint32_t ZutexTable::wake(uint32_t guest_zutex, Zutex_count n_wake)
{
	uint32_t result;
	_lock();
#if DEBUG_DISASTER
	dbg_sanity("wake");
#endif //DEBUG_DISASTER
	ZutexQueue *zq = _get_queue(guest_zutex);
	if (zq==NULL)
	{
		// nothing to wake!
		result = 0;
	}
	else
	{
		ZutexQueue private_wake_queue(mf, 0);
		// select the n_wake victims
		for (Zutex_count i=0; i<n_wake; i++)
		{
			ZutexWaiter *waiter = zq->pop();

			if (waiter==NULL)
			{
				break;
			}

#if DEBUG_DISASTER
			fprintf(stderr, "wake: popped W#%d from ZQ@%08x\n",
				waiter->dbg_get_vcpu_zid(), zq->dbg_get_guest_hdl());
			dbg_sanity("after pop");
#endif //DEBUG_DISASTER

			private_wake_queue.push(waiter);
		}

		HashTable empty_zq_set;
		hash_table_init(&empty_zq_set, mf, ZutexQueue::hash, ZutexQueue::cmp);

		uint32_t num_woken = 0;
		while (true)
		{
			ZutexWaiter *waiter = private_wake_queue.pop();
			if (waiter==NULL)
			{
				break;
			}

			LinkedListIterator lli;
			for (ll_start(waiter->get_zq_list(), &lli);
				ll_has_more(&lli);
				ll_remove(waiter->get_zq_list(), &lli))
			{
				ZutexQueue *wq = (ZutexQueue *) ll_read(&lli);
#if DEBUG_DISASTER
				fprintf(stderr, "Inspecting ZQ@%08x from W#%d; it now has %d items:\n",
					wq->dbg_get_guest_hdl(),
					waiter->dbg_get_vcpu_zid(),
					waiter->get_zq_list()->count);
				dbg_dump_waiter(waiter);
#endif //DEBUG_DISASTER

				if (wq!=zq)
				{
#if DEBUG_DISASTER
					fprintf(stderr, "wake: removes W#%d from ZQ@%08x\n",
						waiter->dbg_get_vcpu_zid(), wq->dbg_get_guest_hdl());
#endif //DEBUG_DISASTER
					wq->remove(waiter);
#if DEBUG_DISASTER
					dbg_sanity("after remove");
#endif //DEBUG_DISASTER
				}
				// Arrange to garbage collect queues that have emptied
				if (wq->is_empty())
				{
					if (hash_table_lookup(&empty_zq_set, wq)==NULL)
					{
						hash_table_insert(&empty_zq_set, wq);
					}
				}
			}
			waiter->wake();
			num_woken += 1;
		}
		result = num_woken;

		hash_table_remove_every(&empty_zq_set, _free_zq, this);
		hash_table_free(&empty_zq_set);
	}
	_unlock();
	return result;
}
