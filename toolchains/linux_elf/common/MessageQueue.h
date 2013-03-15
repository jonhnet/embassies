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
#include "ZeroCopyBuf.h"

typedef struct s_mqentry {
	struct s_mqentry *next;
	ZeroCopyBuf *zcb;
//	ZoogNetBuffer *xnb;
	IPInfo info;	// parse it just once
} MQEntry;

typedef struct s_messagequeue {
	ZAS zas;	// supeclass field; needs to be at the beginning of struct.
				// Ugh. Probably time to break out the C++.
	ZMutex zmutex;
	char dbg_comment;
	MQEntry head;
	uint32_t seq_zutex;
	ZoogDispatchTable_v1 *xdt;
} MessageQueue;

void mq_init(MessageQueue *mq, ZoogDispatchTable_v1 *xdt);
void mq_append(MessageQueue *mq, MQEntry *msg);
MQEntry *mq_pop(MessageQueue *mq, bool block);

