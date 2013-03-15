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
#include "NavigationProtocol.h"
#include "hash_table.h"
#include "pal_abi/pal_abi.h"
#include "SyncFactory_Pthreads.h"
#include "NavIfcs.h"

class NavMgr;
class AwayAction;

class NavInstance : public NavCalloutIfc {
private:
	NavMgr *nav_mgr;
	NavigateIfc *nav_ifc;
	EntryPointIfc* entry_point;
	NavigationBlob *back_token;

	bool pane_visible;
	AwayAction *_action_pending;
	ViewportID _tab_tenant;
	DeedKey _tab_deed_key;
	ZCanvas _tab_canvas;
	ViewportID _pane_tenant;

	pthread_t away_2_pt;

	void unpack_prev_token(NavigationBlob *msg);
	void map_tab(Deed tab_deed);
	void _paint_canvas(ZCanvas *zcanvas);
	void transmit_acknowledge(uint64_t rpc_nonce);
	void transmit_transfer_complete();
	void away(AwayAction *aa);
	void _start_away_2(AwayAction *aa);
	ZoogDispatchTable_v1* _zdt();

	uint8_t *load_all(const char *path);
	SignedBinary* lookup_boot_block(const char *vendor_name);
	ZPubKey* pub_key_from_signed_binary(SignedBinary* sb);

public:
	NavInstance(
		NavMgr *nav_mgr, NavigateIfc *nav_ifc, EntryPointIfc *entry_point,
		NavigationBlob *back_token, Deed _tab_deed, uint64_t _rpc_nonce);

	virtual void transfer_forward(const char *vendor_name, EntryPointIfc* entry_point);
		// this copies vendor_name, keeps entry_point
	virtual void transfer_backward();

	// for NavMgr
	void pane_revealed(Deed pane_deed);
	void pane_hidden();
	DeedKey get_tab_deed_key() { return _tab_deed_key; }

	// for AwayAction
	NavigationBlob *get_outgoing_back_token();
	void _away_2(AwayAction *aa);
	const char *get_vendor_name_from_hash(hash_t vendor_id);
};


