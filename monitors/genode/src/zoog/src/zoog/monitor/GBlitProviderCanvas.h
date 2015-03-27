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

#include <nitpicker_session/connection.h>
//#include <nitpicker_gfx/chunky_canvas.h>
//#include <nitpicker_gfx/pixel_rgb565.h>
#include <nitpicker_view/client.h>
#include <timer_session/connection.h>

#include "BlitProviderIfc.h"
#include "GCanvasAcceptorIfc.h"
#include "KeyTranslate.h"

class GBlitProvider;

class GBlitProviderCanvas
	: public BlitProviderCanvasIfc {
private:
	class InputHandlerThread : public Genode::Thread<8192>
	{
	private:
		GBlitProviderCanvas *gblit_canvas;
		void entry()
			{ gblit_canvas->run_input_handler_thread(); }
	public:
		InputHandlerThread(GBlitProviderCanvas *gblit_canvas)
			: gblit_canvas(gblit_canvas) {}
	};

	ZCanvasID canvas_id;

	ZRectangle wpos;
	Nitpicker::Connection *nitpicker;
	Nitpicker::View_capability _cap;
	
//	Chunky_canvas<Pixel_rgb565> canvas;
	Timer::Connection *timer;
	InputHandlerThread input_handler_thread;
	ProviderEventDeliveryIfc *event_delivery_ifc;

	KeyTranslate key_translate;

	void move(int x, int y);
	void top();
	void run_input_handler_thread();

	int x() { return wpos.x; }
	int y() { return wpos.y; }

	friend class InputHandlerThread;
	
	void open_nitpicker_window(ZRectangle *wr, const char* window_label);

public:
	GBlitProviderCanvas(
		GBlitProvider *gblit_provider,
		ZCanvasID canvas_id,
		ZRectangle *wr,
		const char* window_label,
		GCanvasAcceptorIfc *gacceptor,
		ProviderEventDeliveryIfc *event_delivery_ifc,
		GBlitProviderCanvas *parent_gbc,
		Timer::Connection *timer);

	// BlitProviderCanvasIfc
	virtual ~GBlitProviderCanvas();

	virtual uint32_t hash();
	virtual int cmp(Hashable *other);

	virtual ZCanvasID get_canvas_id();
	virtual void update_image(ZRectangle *wr);
	virtual void reconfigure(ZRectangle *wr);

};

