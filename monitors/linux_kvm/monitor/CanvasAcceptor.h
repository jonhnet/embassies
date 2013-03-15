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

class ZoogVM;

class CanvasAcceptor
{
private:
	ZoogVM *vm;
	ViewportID viewport_id;
	PixelFormat known_formats[4];
	int num_formats;

	ZCanvas *out_canvas;
	SyncFactoryEvent *event;
	MemSlot *slot;

public:
	CanvasAcceptor(
		ZoogVM *vm,
		ViewportID viewport_id,
		PixelFormat *known_formats,
		int num_formats,
		ZCanvas *out_canvas);
	CanvasAcceptor(ViewportID viewport_id); // key ctor
	~CanvasAcceptor();

	// upcalls from CoordinatorConnection
	ViewportID get_viewport_id() { return viewport_id; }
	PixelFormat *get_known_formats() { return known_formats; }
	int get_num_formats() { return num_formats; }
	void *allocate_memory_for_incoming_canvas(uint32_t capacity);
	void signal_canvas_ready(ZCanvas *received_canvas);

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

	// ifc from ZoogVCPU
	void wait();
};
