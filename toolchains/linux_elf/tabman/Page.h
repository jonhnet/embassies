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

#include "pal_abi/pal_abi.h"
#include "NavigationBlob.h"
#include "NavigationProtocol.h"
#include "ZoogBox.h"

class Page {
private:
	ZoogDispatchTable_v1 *zdt;
	uint32_t coloridx;

	GtkWidget *notebook;
	TabBox *tab_box;
	PaneBox *pane_box;

	static void tab_go_cb(GtkWidget *widget, gpointer data);
	void tab_go();
	gint find_my_page_num();

public:
	Page(ZoogDispatchTable_v1 *zdt, GtkWidget *notebook, uint32_t coloridx);
	ZoogDispatchTable_v1* get_zdt() { return zdt; }
	uint32_t get_coloridx() { return coloridx; }

	void map_pane();
	void unmap_pane();
	void tab_transfer_start();
	void tab_transfer_complete();
	bool has_tab_key(DeedKey key);
	void refresh_tab_key();
};
