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

#if 0
extern "C" {
#include "ppm.h"
}
#endif
#include "png.h"

#include "pal_abi/pal_abi.h"

class TabIconPainter
{
private:
	int cols, rows;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep* row_pointers;

	void load(const char *icon_path);
public:
	TabIconPainter();
	TabIconPainter(const char *icon_path);
	~TabIconPainter();

	void paint_canvas(ZCanvas *zcanvas);
};
