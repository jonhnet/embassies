#include <dataspace/client.h>

#include "NetBufferTable.h"
#include "AllocatedBuffer.h"
#include "standard_malloc_factory.h"

using namespace ZoogPal;

NetBufferTable::NetBufferTable() {
	hash_table_init(
		&_ht,
		standard_malloc_factory_init(),
		&AllocatedBuffer::hash,
		&AllocatedBuffer::cmp);
}

ZoogNetBuffer *NetBufferTable::insert(Dataspace_capability buffer_cap) {
	AllocatedBuffer *allocated_buffer = new AllocatedBuffer(buffer_cap);
	_lock.lock();
	hash_table_insert(&_ht, allocated_buffer);
	_lock.unlock();
	return allocated_buffer->znb();
}

ZoogMonitor::Session::Zoog_genode_net_buffer_id NetBufferTable::retrieve(ZoogNetBuffer *znb, AllocatedBuffer **out_buffer, const char *state) {
//	PDBG("retrieve digging into znb @%08x\n", znb);
	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id =
		((ZoogMonitor::Session::Zoog_genode_net_buffer_id *) znb)[-1];
//	PDBG("retrieve found buffer_id %d\n", buffer_id);
	AllocatedBuffer key(buffer_id);
	_lock.lock();
	AllocatedBuffer *allocated_buffer =
		(AllocatedBuffer *) hash_table_lookup(&_ht, &key);
	_lock.unlock();
	if (allocated_buffer == NULL) {
//		uint32_t ebp; __asm__("mov %%ebp,%0" : "=m"(ebp));
//		PDBG("hash table lookup failed %08x", ebp);
		PDBG("hash table lookup for buffer_id 0x%x failed via %s", buffer_id, state);
	}
	if (out_buffer != NULL) {
		*out_buffer = allocated_buffer;
	}
	return buffer_id;
}

ZoogMonitor::Session::Zoog_genode_net_buffer_id NetBufferTable::lookup(ZoogNetBuffer *znb, const char *state) {
	return retrieve(znb, NULL, state);
}

void NetBufferTable::remove(ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id) {
	AllocatedBuffer key(buffer_id);
	_lock.lock();
	AllocatedBuffer *allocated_buffer =
		(AllocatedBuffer *) hash_table_lookup(&_ht, &key);
	hash_table_remove(&_ht, allocated_buffer);
	_lock.unlock();
	if (allocated_buffer == NULL) {
		PDBG("hash table lookup failed");
	}
	delete allocated_buffer;
}

void NetBufferTable::enumerate_znb_foreach(void *v_list, void *v_ab)
{
	LinkedList *list = (LinkedList *) v_list;
	AllocatedBuffer *ab = (AllocatedBuffer *) v_ab;
	linked_list_insert_tail(list, ab->znb());
}

void NetBufferTable::enumerate_znbs(LinkedList *list)
{
	_lock.lock();
	hash_table_visit_every(&_ht, enumerate_znb_foreach, list);
	_lock.unlock();
}
