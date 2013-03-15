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
#include <nitpicker_gfx/chunky_canvas.h>
#include <nitpicker_gfx/pixel_rgb565.h>
#include <nitpicker_view/client.h>
#include <timer_session/connection.h>

#include "BlitProviderIfc.h"

class GBlitProvider : public BlitProviderIfc {
private:
	Timer::Connection *timer;

public:
	GBlitProvider(
		Timer::Connection *timer);

	BlitProviderCanvasIfc *create_canvas(
		ZCanvasID canvas_id,
		WindowRect *wr,
		BlitCanvasAcceptorIfc *canvas_acceptor,
		ProviderEventDeliveryIfc *event_delivery_ifc,
		BlitProviderCanvasIfc *parent);
};


