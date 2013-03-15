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

#include "CoordinatorProtocol.h"
#include "LongMessageAllocator.h"

class Packet
{
public:
	Packet(uint32_t cm_size, CMDeliverPacket *dp);
	Packet(LongMessageAllocation* lma);
	~Packet();

	uint32_t get_size();
	uint8_t *get_data();
	LongMessageAllocation* get_lma();
	uint32_t get_dbg_size();

private:
	uint32_t size;
	uint8_t *data;
	LongMessageAllocation* lma;
};
