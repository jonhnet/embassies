#include "standard_malloc_factory.h"
#include "PacketQueue.h"

using namespace ZoogMonitor;

Packet_queue::Packet_queue()
{
	linked_list_init(&queue, standard_malloc_factory_init());
}

void Packet_queue::insert(Packet *packet)
{
	lock.lock();
	linked_list_insert_tail(&queue, packet);
//	semaphore.up();
	lock.unlock();
}

Packet *Packet_queue::remove()
{
//	PDBG("down");
//	semaphore.down();
//	PDBG("done");
	lock.lock();
	Packet *result = (Packet *) linked_list_remove_head(&queue);
	lock.unlock();
	return result;
}

