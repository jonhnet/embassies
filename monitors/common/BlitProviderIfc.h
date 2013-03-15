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

#include "Hashable.h"
#include "UIEventDeliveryIfc.h"
#include "BlitPalIfc.h"

class BlitProviderCanvasIfc
	: public Hashable
{
public:
	virtual ~BlitProviderCanvasIfc() {}

	virtual ZCanvasID get_canvas_id() = 0;
	virtual void update_image(ZRectangle *wr) = 0;
	virtual void reconfigure(ZRectangle *wr) = 0;
};

class ProviderEventDeliveryIfc : public UIEventDeliveryIfc {
public:
	virtual void notify_canvas_closed(BlitProviderCanvasIfc *pcanvas) = 0;
};

class BlitProviderIfc
{

public:
	virtual BlitProviderCanvasIfc *create_canvas(
		ZCanvasID canvas_id,
		ZRectangle *wr,
		BlitCanvasAcceptorIfc *canvas_acceptor,
		ProviderEventDeliveryIfc *event_delivery_ifc,
		BlitProviderCanvasIfc *parent,
		const char *window_label) = 0;
};

