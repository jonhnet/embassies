#include "ThreadFactory_Cheesy.h"

ThreadFactory_Cheesy::ThreadFactory_Cheesy(ZoogDispatchTable_v1 *xdt)
{
	this->xdt = xdt;
}

void ThreadFactory_Cheesy::create_thread(cheesythread_f *func, void *arg, uint32_t stack_size)
{
	cheesy_thread_create(xdt, func, arg, stack_size);
}
