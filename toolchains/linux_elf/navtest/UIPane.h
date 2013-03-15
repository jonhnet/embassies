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

#include <pal_abi/pal_abi.h>
#include "NavIfcs.h"

class NavTest;

class UIPane : public NavigateIfc {
private:
	NavTest* nav_test;
	NavCalloutIfc *callout_ifc;
	uint32_t _color;
	ZCanvas _pane_canvas;

	void _paint_canvas(ZCanvas *zcanvas);
	ZoogDispatchTable_v1* _zdt();

public:
	UIPane(NavTest* nav_test);

	bool has_canvas(ZCanvasID canvas_id);

	virtual void setup_callout_ifc(NavCalloutIfc *callout_ifc);
	virtual void navigate(EntryPointIfc *entry_point);
	virtual void no_longer_used();
	virtual void pane_revealed(ViewportID pane_tenant_id);
	virtual void pane_hidden();

	void forward();
	void back();
	void new_tab();
};

