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

#include "linked_list.h"
#include "Page.h"

class TabMan {
private:
    GtkWidget *window;
    GtkWidget *notebook;
    
	LinkedList pages;
	ZoogDispatchTable_v1 *zdt;
	uint32_t next_color;
	bool mapped;
	Page *last_page;
	int nav_socket;

	void _set_up_navigation_protocol_socket();
	static void _read_navigation_protocol_socket_cb(gpointer data, gint source, GdkInputCondition condition);
	void _read_navigation_protocol_socket();
	void _tab_transfer_start(NavigationBlob *msg);
	void _tab_transfer_complete(NavigationBlob *msg);
	Page* _map_tab_key_to_page(DeedKey key);
	void _refresh_all_page_tab_deed_keys();

	static void _window_mapped_cb(GtkWidget *widget, GdkEvent *event, gpointer data);
	void _window_mapped();
	static void new_tab_cb(GtkWidget *widget, gpointer data);
	static void switch_page_cb(GtkNotebook *notebook, gpointer ptr, guint page, gpointer data);
	void switch_page(GtkNotebook *notebook, guint page_num);
	void new_tab();

public:
	TabMan();
};
