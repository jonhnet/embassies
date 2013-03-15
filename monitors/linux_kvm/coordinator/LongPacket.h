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

#include "Packet.h"

class LongPacket : public Packet
{
public:
	LongPacket(CMDeliverPacketLong*, LongMessageAllocation* lma);
	~LongPacket();

	virtual XIPAddr *get_dest() { return &ipinfo.dest; }

	CMDeliverPacket *get_as_cm() { return NULL; }
	uint32_t get_cm_size() { lite_assert(false); return 0; }

	virtual LongMessageAllocation* as_lma() { return lma; }
	
	virtual void *get_payload() { lite_assert(false); return NULL; }
	virtual bool packet_came_from_tunnel() { return false; }
	
private:
	LongMessageAllocation* lma;
};
