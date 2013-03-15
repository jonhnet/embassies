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

#include "SyncFactory.h"
#include "linked_list.h"
#include "pal_abi/pal_ui.h"
#include "BlitPalIfc.h"
#include "BlitProviderIfc.h"

class BlitterViewport;
class BlitterManager;

class BlitterCanvas
	: public ProviderEventDeliveryIfc
{
private:
	BlitterManager *blitter_mgr;
	BlitPalIfc *palifc;
	BlitterViewport *viewport;
	BlitProviderIfc *provider;
	UIEventDeliveryIfc *uidelivery;
	BlitProviderCanvasIfc *pc;
	ZCanvasID canvas_id;

public:
	BlitterCanvas(
		BlitterManager *blitter_mgr,
		BlitPalIfc *palifc,
		BlitterViewport *viewport,
		PixelFormat *known_formats,
		int num_formats,
		BlitCanvasAcceptorIfc *acceptor,
		const char *window_label);

	BlitterCanvas(ZCanvasID canvas_id);	// key ctor

	~BlitterCanvas();
		// don't call directly; only for key case.
		// (Wish I could make it private; too bad my hash tables stink.)

	void unmap();

	void update_canvas(ZRectangle *wr);

#if 0
	ViewportID delegate_viewport(ZPubKey *vendor);

	void update_viewport(ZRectangle *wr);
#endif

	// ProviderEventDeliveryIfc
	void deliver_ui_event(ZoogUIEvent *zuie);
	void notify_canvas_closed(BlitProviderCanvasIfc *pc);

	BlitProviderCanvasIfc *get_pc() { return pc; }
	BlitPalIfc *get_palifc() { return palifc; }
	ZCanvasID get_canvas_id() { return canvas_id; }

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

