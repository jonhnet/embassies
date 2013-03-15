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

// This object represents one live app
// (Um, how would we know when it has died?)

#include "pal_abi/pal_abi.h"
#include "Hashable.h"
#include "Message.h"
#include "AppIfc.h"
#include "Xblit.h"
#include "BlitPalIfc.h"

class Listener;

class App
	: public Hashable,
	  public BlitPalIfc,
	  public UIEventDeliveryIfc
{
private:
	void _create_ifconfig(XIPVer ipver, XIPifconfig *ifconfig, int tunnel_address);
	ZPubKey *pub_key;
	bool verified;
	Listener *listener;

	Message *connect_message;
		// Gives us remote addr to send outgoing messages to.

	bool destructing;
		// avoid recursion during destruction

	HashTable outstanding_lmas;
		// keep track of lmas the client leaked.

	XIPifconfig ifconfigs[2];

	void send_long_mmap(CMHeader *real, uint32_t real_len, CMHeader *ref_message, uint32_t ref_length);
	void send_long_shm(CMHeader *real, uint32_t real_len, CMHeader *ref_message, uint32_t ref_length);
	void clean_up_outstanding_messages();
	static void _clean_up_one_s(void *v_this, void *v_lma);
	

	enum ShmMode { USE_MMAP, USE_SHM};
	static const ShmMode mode;

public:
	App(Listener *listener, ZPubKey *pub_key, Message *connect_message, int tunnel_address);
	~App();
	void send_cm(CMHeader *header, uint32_t length);
	void send_packet(Packet *p);
	void record_alloc_long_message(LongMessageAllocation* lma);
	void free_long_message(CMFreeLongMessage *msg);
	XIPAddr *get_ip_addr(XIPVer ver);

	virtual uint32_t hash();
	virtual int cmp(Hashable *other);

	virtual void deliver_ui_event(ZoogUIEvent *event);
	virtual void debug_remark(const char *msg);

	UIEventDeliveryIfc *get_uidelivery();
	virtual ZPubKey *get_pub_key();
	void verify_label() { verified = true; }
	const char *get_label();
	BlitPalIfc *as_palifc() { return (BlitPalIfc*) this; }
	uint32_t get_id() { return (uint32_t) this; }
};
