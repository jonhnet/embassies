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
#include "ViewportHandle.h"

class BlitterCanvas;
class BlitterManager;

class BlitterViewport {
private:
	BlitterManager *blitter_mgr;
	ViewportHandle *landlord_handle;
	ViewportHandle *tenant_handle;
	Deed deed;	// tenant_handle!=NULL XOR a deed is outstanding
	DeedKey deed_key;	// valid when deed is valid
	BlitterViewport *parent_viewport;
		// if NULL, this is a top-level viewport, and its canvas
		// should be a top-level window in the host environment.
	BlitterCanvas *mapped_canvas;

	// stashed values of viewport configuration, for use when
	// canvas is accepted.
	// (TODO this is a little clumsy, all because we don't create the
	// X window until the canvas is accepted, so we can do the shm thing
	// once we know who the recipient PAL palifc is. But maybe we should
	// create the window first thing, and then attach the shm once its
	// available?)
	ZRectangle window_configuration;

	static uint32_t next_viewport_id;

public:
	BlitterViewport(
		BlitterManager *blitter_mgr,
		uint32_t process_id,
		BlitterViewport *parent_viewport,
		ZRectangle *rectangle);

//	BlitterViewport(ViewportID viewport_id);	// key ctor
	~BlitterViewport();

	void map_canvas(
		BlitPalIfc *palifc,
		PixelFormat *known_formats,
		int num_formats,
		BlitCanvasAcceptorIfc *acceptor,
		const char *window_label);

	void unmap_canvas();

	void update_viewport(ZRectangle *wr);
	ViewportID get_landlord_id();
	Deed get_deed();
	DeedKey get_deed_key();
	ViewportID get_tenant_id();
	BlitProviderCanvasIfc *get_parent_canvas();
#if 0
	BlitPalIfc *get_palifc();
#endif
	ZRectangle *get_window_configuration() { return &window_configuration; }
	void canvas_closed(BlitterCanvas *canvas);

	Deed transfer();
	void accept_viewport(uint32_t process_id);
	BlitterViewport* sublet(ZRectangle *rectangle, uint32_t process_id);

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

