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
// TODO should refactor; almost identical to linux_kvm implementation.
// varies only because we store the table inside the PAL here, vs.
// out in the monitor there.

#pragma once

#include "pal_abi/pal_zutex.h"
#include "ZutexTable.h"
#include "EventSynchronizerIfc.h"

class HostAlarms
	: public EventSynchronizerIfc
{
private:
	ZutexTable *zutex_table;
	ZoogHostAlarms the_alarms;
	uint32_t *the_alarms_ary;

public:
	HostAlarms(ZutexTable *zutex_table);
	void signal(AlarmEnum alarm);
	uint32_t get_guest_addr(AlarmEnum alarm);
	void update_event(AlarmEnum alarm, uint32_t sequence_number);
	ZoogHostAlarms *get_zoog_host_alarms() { return &the_alarms; }
};
