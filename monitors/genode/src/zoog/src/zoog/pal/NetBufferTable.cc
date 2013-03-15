#include <dataspace/client.h>

#include "NetBufferTable.h"
#include "standard_malloc_factory.h"

using namespace ZoogPal;

ZoogMonitor::Session::Zoog_genode_net_buffer_id AllocatedBuffer::buffer_id() {
	return _buffer_id;
}

ZoogNetBuffer *AllocatedBuffer::znb() {
	return (ZoogNetBuffer *)
		&((ZoogMonitor::Session::Zoog_genode_net_buffer_id *) _guest_mapping)[1];
}

uint32_t AllocatedBuffer::hash(const void *v_a) {
	return ((AllocatedBuffer*)v_a)->_buffer_id;
}

int AllocatedBuffer::cmp(const void *v_a, const void *v_b) {
	AllocatedBuffer *a = (AllocatedBuffer *) v_a;
	AllocatedBuffer *b = (AllocatedBuffer *) v_b;
	int result = (a->_buffer_id) - (b->_buffer_id);
//	PDBG("cmp %08x,%08x : %d, %d giving %d",
//		v_a, v_b, a->_buffer_id, b->_buffer_id, result);
	return result;
}

AllocatedBuffer::AllocatedBuffer(Dataspace_capability buffer_cap)
	: _buffer_cap(buffer_cap)
{
	Dataspace_client ds_client(_buffer_cap);
	_guest_mapping = env()->rm_session()->attach(ds_client);
	_buffer_id = ((ZoogMonitor::Session::Zoog_genode_net_buffer_id *) _guest_mapping)[0];
//	PDBG("Mapped   buffer id %x at %p", _buffer_id, _guest_mapping);
}

AllocatedBuffer::AllocatedBuffer(ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id)
	: _buffer_id(buffer_id),
	  _guest_mapping(NULL)
{ }

AllocatedBuffer::~AllocatedBuffer() {
}


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
