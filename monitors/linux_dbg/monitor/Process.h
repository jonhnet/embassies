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

#include "zq.h"
#include "xpbpool.h"
#include "netifc.h"
#include "EventSynchronizer.h"
#include "BlitPalIfc.h"
#include "ZPubKey.h"
#include "MThread.h"
#include "UIEventQueue.h"

class Monitor;

#if DEBUG_ZUTEX_OWNER
// Define this symbol in makefile (also affects zq.c) if you need
// to track down a zutex deadlock.
// Also affects _pal.
#define DECLARE_ZLO ZQLockOwnerInfo zlo = { pthread_self(), tx->xph->thread_id };
#define	TRANSMIT_ZLO	,&zlo
typedef struct {
	pthread_mutex_t mutex;
	FILE *ofp;
} DebugZutexThing;

void dzt_init(DebugZutexThing *dzt);
void dzt_log(DebugZutexThing *dzt, XpHeader *xph, XaxFrame *xf, const char *msg);
#else

#define DECLARE_ZLO 	/**/
#define	TRANSMIT_ZLO	/**/
#endif // DEBUG_ZUTEX_OWNER

class Process : public BlitPalIfc {
private:
	Monitor *m;
	uint32_t process_id;

	XPBPool *xpbpool;

	EventSynchronizer *event_synchronizer;
	AppInterface app_interface;
	ThreadTable *thread_table;
	ZQTable *zqt;
	UIEventQueue *uieq;

	ZQ *pool_full_queue;	// threads waiting for XNBs, but pool is full. (untested path)
	pthread_mutex_t pool_full_queue_mutex;

	ZPubKey* pub_key;   // Vendor's identity
	bool labeled;   // Have we successfully verified the vendor's name/label?

#if DEBUG_ZUTEX_OWNER
	DebugZutexThing dzt;
#endif // DEBUG_ZUTEX_OWNER

public:
	Process(Monitor *m, uint32_t process_id, ZPubKey *pub_key);
	Process(uint32_t process_id);	// key ctor
	~Process();

	XPBPool *get_buf_pool() { return xpbpool; }
	AppInterface *get_app_interface() { return &app_interface; }

	MThread *lookup_thread(int thread_id);
	ZQ *lookup_zq(uint32_t *zutex);

	// BlitPalIfc 
	UIEventDeliveryIfc *get_uidelivery() { return uieq; }
	ZPubKey *get_pub_key() { return pub_key; }
	void debug_remark(const char *msg) { fprintf(stderr, "%s\n", msg); }
	
	UIEventQueue *get_uieq() { return uieq; }
	void mark_labeled();
	const char *get_label();

	EventSynchronizer *get_event_synchronizer() { return event_synchronizer; }

	uint32_t dbg_get_process_id() { return process_id; }
	uint32_t get_id() { return process_id; }
		// process_id became first class when we began using it to sort
		// out different handle owners in BlitterManager

	bool pid_is_live();

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

	BlitPalIfc *as_palifc() { return this; }
};

