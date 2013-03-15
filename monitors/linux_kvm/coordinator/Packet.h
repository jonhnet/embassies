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

#include "xax_network_utils.h"
#include "Message.h"

class Packet
{
public:
	virtual XIPAddr *get_dest() { return &ipinfo.dest; }
	virtual ~Packet() {}

	// only valid for MonitorPackets
	virtual CMDeliverPacket *get_as_cm() = 0;
	virtual uint32_t get_cm_size() = 0;

	// only valid for LongPackets
	virtual LongMessageAllocation* as_lma() = 0;

	virtual void *get_payload() { return payload; }
	bool valid() { return payload!=NULL; }
	uint32_t get_size() { return size; }
	virtual bool packet_came_from_tunnel() = 0;

protected:
	Packet();
	void _construct(void *payload, uint32_t size);

protected:
	IPInfo ipinfo;
	uint32_t size;

	// TODO this field unused in LongPacket; need class hierarchy refactor:
	void *payload;
};
