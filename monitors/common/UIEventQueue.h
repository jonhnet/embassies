/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "pal_abi/pal_ui.h"
#include "EventSynchronizerIfc.h"
#include "SyncFactory.h"
#include "linked_list.h"
#include "UIEventDeliveryIfc.h"

class UIEventQueue : public UIEventDeliveryIfc {
public:
	UIEventQueue(MallocFactory *mf, SyncFactory *sf, EventSynchronizerIfc *event_synchronizer);
	~UIEventQueue();
	void deliver_ui_event(ZoogUIEvent *event);
		// this owns new'd event
	ZoogUIEvent *remove();
		// caller owns return value; should delete

private:
	EventSynchronizerIfc *event_synchronizer;
	SyncFactoryMutex *mutex;
	LinkedList list;
	uint32_t sequence_number;
};
