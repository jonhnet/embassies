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

#include "pal_abi/pal_abi.h"
#include "NavigationBlob.h"

// Anonymous messages between TabManager and vendors that paint tab panes
struct TMPaneHidden {
	enum { OPCODE = 2 };
	DeedKey tab_deed_key;
};

struct TMPaneRevealed {
	enum { OPCODE = 3 };
	DeedKey tab_deed_key;
	Deed pane_deed;
};

struct TMNewTabRequest {
	enum { OPCODE = 4 };
};

struct TMNewTabReply {
	enum { OPCODE = 5 };
	Deed tab_deed;
};

// Non-anonymous messages between links in a navigation chain
struct NavigateForward {
	enum { OPCODE = 6 };
	hash_t dst_vendor_id_hash;
	uint64_t rpc_nonce;
	Deed tab_deed;
	uint8_t entry_point[0];	// NavigationBlob
	uint8_t back_token[0];	// NavigationBlob
		// sender gets to store opaque data in the back_token; perhaps
		// a reference to private storage, or an (encrypted?) stateless
		// entry point.
		// if zero-length, recipient is the first owner of this tab;
		// there is no "back" operation from here.
};

//   The first is a public convention:
// BACK_TOKEN := [] | [ vendor_id : OPAQUE_PREV_DATA ]
//   These are private conventions used inside a particular app;
//   change at whim. E.g. make entry point a session state record.
// OPAQUE_PREV_DATA := [ ENTRY_POINT : BACK_TOKEN ]
// ENTRY_POINT := [url]

struct NavigateBackward {
	enum { OPCODE = 7 };
	hash_t dst_vendor_id_hash;
	uint64_t rpc_nonce;
	Deed tab_deed;
	uint8_t opaque_prev_data[0];
		// NavigationBlob; whatever was in the BackToken's prev_data field
};

struct TMTabTransferStart {
	enum { OPCODE = 8 };
	DeedKey tab_deed_key;
	// Message from tab impl to tabman, says "unmap my pane, if I'm up,
	// and you'll be seeing a Complete message from new guy soon."
};

struct TMTabTransferComplete {
	enum { OPCODE = 9 };
	DeedKey tab_deed_key;
	// Message from new guy to TabMan saying "you can reveal my pane now,
	// if I'm the front tab."
};

struct NavigateAcknowledge {
	enum { OPCODE = 10 };
	uint64_t rpc_nonce;
};

class NavigationProtocol {
public:
	enum { PORT = 50001 };
};
