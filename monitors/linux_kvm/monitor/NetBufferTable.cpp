#include "NetBufferTable.h"
#include "ZoogVM.h"

NetBufferTable::NetBufferTable(ZoogVM *vm, SyncFactory *sf)
{
	this->vm = vm;
	this->mutex = sf->new_mutex(false);
	hash_table_init(&ht, vm->get_malloc_factory(), NetBuffer::hash, NetBuffer::cmp);
}

NetBufferTable::~NetBufferTable()
{
	lite_assert(ht.count==0);	// TODO need cleanup routine.
}

NetBuffer *NetBufferTable::alloc_net_buffer(uint32_t payload_size)
{
	uint32_t mem_required = payload_size + sizeof(ZoogNetBuffer) + KvmNetBufferContainer::PADDING;
	MemSlot *mem_slot = vm->allocate_guest_memory(mem_required, "ZNB");
	NetBuffer *result;
	if (mem_slot==NULL)
	{
		result = NULL;
	}
	else
	{
		result = new NetBuffer(mem_slot, NULL);
		result->get_container_host_addr()->padding = KvmNetBufferContainer::PADDING;
		result->update_with_mapped_data();
		mutex->lock();
		hash_table_insert(&ht, result);
		mutex->unlock();
	}
	return result;
}

NetBuffer *NetBufferTable::install_net_buffer(LongMessageAllocation *lma, bool is_new_buffer)
{
	MemSlot *mem_slot = vm->allocate_guest_memory(lma->get_mapped_size(), "ZNB");
	NetBuffer *result;
	if (mem_slot==NULL)
	{
		result = NULL;
		lite_assert(false);	// now we're leaking the LMA
	}
	else
	{
		result = new NetBuffer(mem_slot, lma);
		lma->map(mem_slot->get_host_addr());

		// If we're the first to see this mapped region, then we
		// need to fill in the KvmNetBufferContainer.
		// (Coordinator creates the region but doesn't even map it until
		// we send it back with a packet header in it.)
		if (is_new_buffer)
		{
			result->get_container_host_addr()->padding = KvmNetBufferContainer::PADDING;
		}
		else
		{
			lite_assert(result->get_container_host_addr()->padding == KvmNetBufferContainer::PADDING);
		}

		// Now that it's mapped and the padding declared,
		// we can write down the size
		// where the guest can see it.
		result->update_with_mapped_data();

		// and now the guest_addr is valid, so we can hash it.
		mutex->lock();
		hash_table_insert(&ht, result);
		mutex->unlock();
	}
	return result;
}

NetBuffer *NetBufferTable::lookup_net_buffer(uint32_t guest_addr)
{
	mutex->lock();
	NetBuffer nb_key(guest_addr);
	NetBuffer *nb = (NetBuffer*) hash_table_lookup(&ht, &nb_key);
	mutex->unlock();
	return nb;
}

void NetBufferTable::free_net_buffer(NetBuffer *net_buffer)
{
	mutex->lock();
	hash_table_remove(&ht, net_buffer);
	mutex->unlock();
	MemSlot *mem_slot = net_buffer->claim_mem_slot();
	vm->free_guest_memory(mem_slot);
		// The mem_slot belongs to the ZoogVM
		// object, so we don't delete it explicitly here.
		// Instead, the free_guest_memory call, via the refcount mechanism,
		// deletes the mem_slot object itself.
	delete net_buffer;
}


