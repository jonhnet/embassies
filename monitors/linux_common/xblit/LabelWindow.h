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

#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

class Xblit;
class XblitCanvas;

class CtorWork;
// class ShowWork;
// class HideWork;

class LabelWindow
{
private:
	Xblit *xblit;
	Window win;
	GC gc;
	XFontStruct *fontinfo;
	char *label_string;

	// X-thread methods
	void _ctor();
//	void _show_inner(XblitCanvas *parent_xbc);
//	void _hide_inner();

	friend class CtorWork;
//	friend class ShowWork;
//	friend class HideWork;

	void _create_window();
	int width;
	int height;

	Colormap screen_colormap;
	XColor green, yellow;
	unsigned long black;
	unsigned long white;

	void _set_label(const char *str);

public:
	LabelWindow(Xblit *xblit);
	void x_show(XblitCanvas *parent_xbc);
	void x_hide();

	Window get_window() { return win; }
	void paint();
};
