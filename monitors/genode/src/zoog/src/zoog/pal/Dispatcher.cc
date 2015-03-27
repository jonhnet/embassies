#include "Dispatcher.h"
#include "Genode_pal.h"

Dispatcher::Dispatcher(GenodePAL *pal, ZoogMonitor::Session::DispatchOp opcode)
	: pal(pal)
{
	dispatch = pal->assign_dispatch_slot(&idx);
	dispatch->opcode = opcode;
}

Dispatcher::~Dispatcher()
{
	pal->dispatch_slot_used[idx] = false;
}

void Dispatcher::rpc()
{
	//PDBG("rpc(%d)", dispatch->opcode);
	pal->zoog_monitor.dispatch_message(idx);
}

