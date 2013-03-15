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
#include "SyncFactory.h"
#include "linux_kvm_protocol.h"

class ZoogVM;

class ViewportDelegator
{
private:
	uint32_t rpc_nonce;

	xa_delegate_viewport *xa;	// in-monitor copy, not race-able by app
	uint8_t *pub_key;			// pointer to guest-visible data

	ViewportID out_viewport_id;

	SyncFactoryEvent *event;

	static uint32_t next_rpc_nonce;

public:
	ViewportDelegator(
		ZoogVM *vm,
		xa_delegate_viewport *xa,
		xa_delegate_viewport *xa_guest);
	ViewportDelegator(uint32_t rpc_nonce);	// key ctor
	~ViewportDelegator();

	// upcalls from CoordinatorConnection
	uint32_t get_rpc_nonce() { return rpc_nonce; }
	ZCanvasID get_canvas_id();
	uint32_t get_pub_key_len();
	uint8_t *get_pub_key();

	void signal_reply_ready(ViewportID viewport_id);

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

	// ifc from ZoogVCPU
	ViewportID wait();
};
