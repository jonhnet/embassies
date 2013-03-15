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

#include "malloc_factory.h"
#include "SyncFactory.h"
#include "linked_list.h"
#include "pal_abi/pal_ui.h"
#include "BlitPalIfc.h"
#include "BlitProviderIfc.h"
#include "hash_table.h"
#include "linked_list.h"
#include "ViewportHandle.h"
#include "RandomSupply.h"
#include "GrowBuffer.h"

class BlitterViewport;
class BlitterCanvas;

class BlitterManager {
private:
	BlitProviderIfc *provider;

	MallocFactory *mf;
	SyncFactoryMutex *mutex;
	RandomSupply *random_supply;
	LinkedList palifcs;
	HashTable viewport_handles;
	HashTable deeds;
	HashTable canvases;
	ZCanvasID next_canvas_id;

	GrowBuffer dbg_deeds_log;
	GrowBuffer dbg_canvases_log;

	static void note_canvas_id(void *uo, void *datum);

	friend class CanvasCleanerUpper;

	BlitterViewport *get_viewport(ViewportID viewport_id, uint32_t process_id);

public:
	BlitterManager(BlitProviderIfc *provider_, MallocFactory *mf, SyncFactory *sf, RandomSupply *random_supply);
	~BlitterManager();
	void register_palifc(BlitPalIfc *palifc);
	void unregister_palifc(BlitPalIfc *palifc);
	void multicast_ui_event(ZPubKey *vendor, ZoogUIEvent *evt);
	void add_viewport_handle(ViewportHandle *viewport_handle);
	void remove_viewport_handle(ViewportHandle *viewport_handle);
	BlitterViewport *get_tenant_viewport(ViewportID viewport_id, uint32_t process_id);
	BlitterViewport *get_landlord_viewport(ViewportID viewport_id, uint32_t process_id);
	BlitterViewport *accept_viewport(Deed deed, uint32_t process_id);
	void add_canvas(BlitterCanvas *canvas);
	void remove_canvas(BlitterCanvas *canvas);
	BlitterCanvas *get_canvas(ZCanvasID canvas_id);
		// TODO this should take a process_id, too.
	ViewportID new_toplevel_viewport(uint32_t process_id);
	ZCanvasID allocate_canvas_id();
	void manufacture_deed(BlitterViewport *viewport, Deed *out_deed, DeedKey *out_deed_key);

	BlitProviderIfc *get_provider() { return provider; }
	MallocFactory *get_mf() { return mf; }
};
