#include <sys/mman.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>

#include "ZoogVM.h"
#include "MemSlot.h"

uint32_t MemSlot::round_up_to_page(uint32_t req_size)
{
	return ((req_size-1) | (PAGE_SIZE-1))+1;
}

uint32_t MemSlot::round_down_to_page(uint32_t req_size)
{
	return (req_size & ~(PAGE_SIZE-1));
}

MemSlot::MemSlot(const char *dbg_label)
{
	this->dbg_label = dbg_label;
	this->configured = false;
}

MemSlot::~MemSlot()
{
}

const char *MemSlot::get_dbg_label()
{
	return dbg_label;
}

uint32_t MemSlot::get_guest_addr()
{
	lite_assert(configured);
	return guest_range.start;
}

uint32_t MemSlot::get_size()
{
	lite_assert(configured);
	return guest_range.size();
}

uint8_t *MemSlot::get_host_addr()
{
	lite_assert(configured);
	return host_addr;
}

void MemSlot::configure(Range guest_range, uint8_t *host_addr)
{
	lite_assert(!configured);
	this->guest_range = guest_range;
	this->host_addr = host_addr;
	configured = true;
}
