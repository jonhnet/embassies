#include "UIEventQueue.h"

UIEventQueue::UIEventQueue(MallocFactory *mf, SyncFactory *sf, EventSynchronizerIfc *event_synchronizer)
{
	this->event_synchronizer = event_synchronizer;
	mutex = sf->new_mutex(false);
	linked_list_init(&list, mf);
	sequence_number = 0;
}

UIEventQueue::~UIEventQueue()
{
	delete mutex;
}

void UIEventQueue::deliver_ui_event(ZoogUIEvent *event)
{
	mutex->lock();
	ZoogUIEvent *event_copy = new ZoogUIEvent;
	*event_copy = *event;
	linked_list_insert_tail(&list, event_copy);
	sequence_number+=1;
	event_synchronizer->update_event(alarm_receive_ui_event, sequence_number);
	mutex->unlock();
}

ZoogUIEvent *UIEventQueue::remove()
{
	mutex->lock();
	ZoogUIEvent *zuie = (ZoogUIEvent *) linked_list_remove_head(&list);
	mutex->unlock();
	return zuie;
}
