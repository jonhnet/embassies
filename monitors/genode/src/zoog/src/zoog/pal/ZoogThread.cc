#include <base/printf.h>
#include "ZoogThread.h"
#include "ThreadTable.h"
#include "LiteLib.h"

ZoogThread::ZoogThread(ThreadTable *thread_table, SyncFactory *sf)
	: eip(0),
	  esp(0),
	  thread_table(thread_table),
	  _waiter(standard_malloc_factory_init(), sf),
	  _core_thread_id(core_thread_id_allocator++)
{
	thread_table->insert_me(this);
	lite_memset(&core_regs, 0x88, sizeof(Core_x86_registers));
}

ZoogThread::ZoogThread(void *eip, void *esp, ThreadTable *thread_table, SyncFactory *sf)
	: eip(eip),
	  esp(esp),
	  thread_table(thread_table),
	  _waiter(standard_malloc_factory_init(), sf),
	  _core_thread_id(core_thread_id_allocator++)
{
}

void ZoogThread::entry()
{
	thread_table->insert_me(this);

	__asm__(
		"mov %0,%%esp;"
		"jmp *%1;"
	: /* no output */
	: "r"(esp), "m"(eip)
	: "eax"
	);
}

void ZoogThread::set_core_state()
{
	// ebp may be superfluous: the debugger will use the IP to determine
	// that we're in the bowels of Genode's -O2 code, and rely instead on
	// esp.
	asm(
		"mov %%ebp,%0;"
		"mov 0(%%esp),%%eax;"
		"mov %%eax,%1;"
		"mov %%esp,%2;"
		: "=m"(core_regs.bp),
		  "=m"(core_regs.ip),
		  "=m"(core_regs.sp)
		:
		: "%eax"
	);
}

void ZoogThread::clear_core_state()
{
	core_regs.ip = 0x88888888;
	core_regs.bp = 0x88888888;
	core_regs.sp = 0x88888888;
}

bool ZoogThread::core_state_valid()
{
	return core_regs.ip != 0x88888888;
}

void ZoogThread::fudge_core_state(uint32_t eip, uint32_t esp)
{
	core_regs.ip = eip;
	core_regs.sp = esp;
	core_regs.bp = esp+4;
		// bp stays borqued, unfortunately. But let's guess we're
		// in frame-pointer code, where return value is one above
		// ... oh, we don't HAVE a frame pointer. That sure stinks.
}

Core_x86_registers *ZoogThread::get_core_regs()
{
	return &core_regs;
}

uint32_t ZoogThread::core_thread_id_allocator = 1;

uint32_t ZoogThread::core_thread_id()
{
	return _core_thread_id;
}
