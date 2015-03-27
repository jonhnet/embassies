#include <dataspace/client.h>

#include "AllocatedBuffer.h"
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
	PDBG("Mapped   buffer id %x at %p", _buffer_id, _guest_mapping);
	PDBG("Znb capacity %d", znb()->capacity);
}

AllocatedBuffer::AllocatedBuffer(ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id)
	: _buffer_id(buffer_id),
	  _guest_mapping(NULL)
{ }

AllocatedBuffer::~AllocatedBuffer() {
}

