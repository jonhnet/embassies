#include "AllocatedBuffer.h"

namespace ZoogMonitor {

Dataspace_capability AllocatedBuffer::cap()
{
	return _dataspace_cap;
}

uint32_t AllocatedBuffer::capacity()
{
	return _allocated_payload_size;
}

Session::Zoog_genode_net_buffer_id AllocatedBuffer::buffer_id()
{
	return ((Session::Zoog_genode_net_buffer_id *) _host_mapping)[0];
}

ZoogNetBuffer* AllocatedBuffer::znb()
{
	return (ZoogNetBuffer *)
		&((Session::Zoog_genode_net_buffer_id *) _host_mapping)[1];
}

void* AllocatedBuffer::payload()
{
	return &znb()[1];
}

uint32_t AllocatedBuffer::hash(const void *v_a)
{
	uint32_t result = ((AllocatedBuffer*)v_a)->_buffer_id;
//			PDBG("hashed %08x giving %d", v_a, result);
	return result;
}

int AllocatedBuffer::cmp(const void *v_a, const void *v_b)
{
	AllocatedBuffer *a = (AllocatedBuffer *) v_a;
	AllocatedBuffer *b = (AllocatedBuffer *) v_b;
	int result = (a->_buffer_id) - (b->_buffer_id);
//			PDBG("cmp %08x,%08x : %d, %d giving %d",
//				v_a, v_b, a->_buffer_id, b->_buffer_id, result);
	return result;
}

AllocatedBuffer::AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id, uint32_t payload_size)
{
	_buffer_id = buffer_id;
	PDBG("allocating some stuff. quota %d used %d\n",
		env()->ram_session()->quota(),
		env()->ram_session()->used());
	_dataspace_cap = env()->ram_session()->alloc(
		sizeof(Session::Zoog_genode_net_buffer_id)
		+ payload_size
		+ sizeof(ZoogNetBuffer));
	PDBG("allocated some stuff; attaching fecally\n");
	_host_mapping = env()->rm_session()->attach(_dataspace_cap);
	PDBG("attached\n");
	((Session::Zoog_genode_net_buffer_id *) _host_mapping)[0] = buffer_id;
	znb()->capacity = payload_size;
	_allocated_payload_size = payload_size;
}

// key ctor
AllocatedBuffer::AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id)
{
	_buffer_id = buffer_id;
	_host_mapping = NULL;
}

AllocatedBuffer::~AllocatedBuffer()
{
	if (_host_mapping==NULL) {
		return;	// this is just a key
	}
	env()->rm_session()->detach(_host_mapping);
	env()->ram_session()->free(_dataspace_cap);
}

}
