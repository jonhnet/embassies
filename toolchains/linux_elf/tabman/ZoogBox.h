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

#include <gtk/gtk.h>
#include "pal_abi/pal_abi.h"
#include "NavigationBlob.h"
#include "NavigationProtocol.h"

class Page;
class PaneBox;

class ZoogBox {
protected:
	enum MaskChange { MASK_SET = 1, MASK_CLEAR = 0 };
	enum MaskBit { EMPTY_MASK = 0, EXPOSE_MASK = 1, SWITCH_PAGE_MASK = 2, TAB_PRESENT_MASK = 4, TAB_NOT_TRANSFERRING = 8 };

	Page *page;
	MaskBit map_mask;

	GtkWidget *eventbox;

	ViewportID landlord_viewport;
	Deed deed;

	ZoogBox(Page *page);
	static void expose_cb(GtkWidget *widget, GtkAllocation *allocation, void *data);
	static void unmap_cb(GtkWidget *widget, void *data);

	virtual void map();
	virtual void unmap();
	virtual void notify_tenant() = 0;
	virtual MaskBit mask_required() = 0;

	void send_msg(NavigationBlob *msg);
	void get_delagation_location(GtkWidget *container, ZRectangle *out_rect);


public:
	GtkWidget *get_widget() { return eventbox; }
	bool is_mapped();
	ViewportID get_landlord_viewport() { return landlord_viewport; }
	void mask_update(MaskChange change, MaskBit mask_bit);
};

class TabBox : public ZoogBox {
private:
	PaneBox *pane_box;

protected:
	virtual void notify_tenant();
	virtual MaskBit mask_required() { return EXPOSE_MASK; }

public:
	TabBox(Page *page);
	void set_pane_box(PaneBox *pane_box);
	virtual void map();
	DeedKey get_deed_key();
	void refresh_deed_key() {}
};

class PaneBox : public ZoogBox {
private:
	TabBox *tab_box;

protected:
	virtual void notify_tenant();
	virtual void map();
	virtual void unmap();
	virtual MaskBit mask_required()
	{ return (MaskBit) (EXPOSE_MASK | SWITCH_PAGE_MASK | TAB_PRESENT_MASK | TAB_NOT_TRANSFERRING ); }

public:
	PaneBox(Page *page, TabBox *tab_box);
	void switch_page(bool enter);
	void transfer(bool transferring);
};
