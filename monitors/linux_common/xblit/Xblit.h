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

#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include "pal_abi/pal_ui.h"
#include "SyncFactory.h"
#include "ThreadFactory.h"
#include "XblitCanvas.h"
#include "hash_table.h"
#include "linked_list.h"
#include "BlitProviderIfc.h"
#include "BlitPalIfc.h"
#include "XWork.h"

class LabelWindow;

class Xblit : public BlitProviderIfc {
private:
	MallocFactory *mf;
	SyncFactory *sf;
	SyncFactoryMutex *x_mutex;	// TODO remove; now x calls all on single thread
	HashTable window_to_xc;
	Display *dsp;
	Atom wmDeleteMessage;
	LabelWindow *label_window;

	LinkedList xwork_queue;
	SyncFactoryMutex *xwork_mutex;
	int xwork_pipes[2];

	Window zoog_focused_window;
	XblitCanvas *focused_canvas;
	int _x_tid;

	Display *_open_display();
	static void _x_worker_s(void *v_this);
	void _x_worker();
	void _map_notify(XMappingEvent *me);
	void _mouse_move(XMotionEvent *me);
	void _key_event(XKeyEvent *xke, ZoogUIEventType which);
	void _mouse_button_event(XButtonEvent *xbe, ZoogUIEventType which);
	void _expose(XExposeEvent *xee);
	void _client_message_event(XClientMessageEvent *xcme);
	XblitCanvas *lookup_canvas_by_window(Window window);
	void _update_zoog_focus(Window win);
	void _focus_in(XFocusChangeEvent *evt);
	void _focus_out(XFocusChangeEvent *evt);
	int _gettid();

public:
	Xblit(ThreadFactory *tf, SyncFactory *sf);
	~Xblit();

	// BlitProviderIfc 
	BlitProviderCanvasIfc *create_canvas(
		ZCanvasID canvas_id,
		ZRectangle *wr,
		BlitCanvasAcceptorIfc *canvas_acceptor,
		ProviderEventDeliveryIfc *event_delivery_ifc,
		BlitProviderCanvasIfc *parent,
		const char *window_label);

	// XblitCanvas accessors
	SyncFactory *get_sync_factory() { return sf; }
//	void lock() { x_mutex->lock(); }
//	void unlock() { x_mutex->unlock(); }
	Display *get_display() { return dsp; }
	Atom *get_wm_delete_message_atom() { return &wmDeleteMessage; }
	void register_canvas(XblitCanvas *bc, Window window);
	void deregister_canvas(XblitCanvas *bc, Window window);
	SyncFactory *get_sf() { return sf; }
	void do_work(XWork *xwork);
};
