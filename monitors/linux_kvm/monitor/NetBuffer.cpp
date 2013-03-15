#include "NetBuffer.h"

// TODO this code is padding EVERY NetBuffer with the stupid page-alignment
// padding intended for jumbo pages. Ugh.

NetBuffer::NetBuffer(MemSlot *mem_slot, LongMessageAllocation *lma)
	: mem_slot(mem_slot),
	  lma(lma)
{
// This ctor can't do anything, since the relevant pages haven't been
// mapped in yet. So we can't update the guest-visible capacity, nor
// do we even know how big the padding is yet.
}

NetBuffer::NetBuffer(uint32_t guest_addr)
	: mem_slot(NULL),
	  lma(NULL)
{
	this->guest_addr = guest_addr;
}

NetBuffer::~NetBuffer()
{
	lite_assert(mem_slot==NULL);
	if (lma!=NULL)
	{
		lma->drop_ref("~NetBuffer");
		lma = NULL;
	}
}

MemSlot *NetBuffer::claim_mem_slot()
{
	MemSlot *result = mem_slot;
	mem_slot = NULL;
	return result;
}

uint32_t NetBuffer::get_guest_znb_addr()
{
	return guest_addr;
}

KvmNetBufferContainer *NetBuffer::get_container_host_addr()
{
	return (KvmNetBufferContainer*) mem_slot->get_host_addr();
}

ZoogNetBuffer *NetBuffer::get_host_znb_addr()
{
	return get_container_host_addr()->get_znb();
}

uint32_t NetBuffer::get_trusted_capacity()
{
	return mem_slot->get_size() - get_container_host_addr()->padding;
}

LongMessageAllocation* NetBuffer::get_lma()
{
	return lma;
}

uint32_t NetBuffer::hash(const void *v_a)
{
	NetBuffer *na = (NetBuffer*) v_a;
	return na->guest_addr;
}

int NetBuffer::cmp(const void *v_a, const void *v_b)
{
	NetBuffer *na = (NetBuffer*) v_a;
	NetBuffer *nb = (NetBuffer*) v_b;
	return na->guest_addr - nb->guest_addr;
}

void NetBuffer::update_with_mapped_data()
{
	get_host_znb_addr()->capacity = get_trusted_capacity();
	guest_addr = mem_slot->get_guest_addr() + get_container_host_addr()->padding;
}
