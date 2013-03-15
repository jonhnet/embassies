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
#include "Hashable.h"
#include "BlitProviderIfc.h"
#include "XBlitCanvasAcceptorIfc.h"

class Xblit;

class CreateWork;
class DestroyWork;
class SetupWork;
class UpdateWork;

class XblitCanvas
	: public BlitProviderCanvasIfc
{
private:
	Xblit *xblit;
	ZCanvasID canvas_id;
	Window win;
	GC gc;
	XImage *backing_image;
	ProviderEventDeliveryIfc *event_delivery_ifc;
	SyncFactoryEvent *window_mapped_event;
	char *_window_label;

	XblitCanvas(Xblit *xblit);
	void _create_window(ZRectangle *wr, XblitCanvas *parent_xbc);
	void _create_window_inner(ZRectangle *wr, XblitCanvas *parent_xbc);
	void _set_up_image(uint32_t width, uint32_t height, uint8_t *data, uint32_t capacity);
	void _set_up_image_inner(uint32_t width, uint32_t height, uint8_t *data, uint32_t capacity);
	void define_zoog_canvas(ZCanvas *canvas);
//	void _update_image_inner(ZRectangle *wr);

public:
	XblitCanvas(
		Xblit *xblit,
		ZCanvasID canvas_id,
		ZRectangle *wr,
		XBlitCanvasAcceptorIfc *xacceptor,
		ProviderEventDeliveryIfc *event_delivery_ifc,
		XblitCanvas *parent_xbc,
		ZCanvas *zoog_canvas);

	// BlitProviderCanvasIfc
	virtual ~XblitCanvas();
	virtual ZCanvasID get_canvas_id() { return canvas_id; }
	void update_image(ZRectangle *wr);
	void reconfigure(ZRectangle *wr);

	// Xblit callins
	void window_mapped();
	void deliver_ui_event(ZoogUIEvent *zuie);
	void x_update_image(ZRectangle *wr);
	void set_window_label(const char *window_label);

	// LabelWindow callin
	void x_get_root_location(ZRectangle *out_rect);
	char *get_window_label();

	virtual uint32_t hash()
		{ return canvas_id; }
	virtual int cmp(Hashable *other)
		{ return canvas_id - ((XblitCanvas *) other)->canvas_id; }

	friend class CreateWork;
	friend class DestroyWork;
	friend class SetupWork;
	friend class UpdateWork;
};


