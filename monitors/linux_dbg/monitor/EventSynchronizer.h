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
#include <pthread.h>
#include "pal_abi/pal_zutex.h"
#include "EventSynchronizerIfc.h"

class EventSynchronizer
	: public EventSynchronizerIfc
{
public:
	EventSynchronizer();
	~EventSynchronizer();
	void update_event(AlarmEnum alarm_num, uint32_t sequence_number);
	void wait(uint32_t *known_sequence_numbers, uint32_t *out_sequence_numbers);

private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	uint32_t sequence_numbers[alarm_count];
};
