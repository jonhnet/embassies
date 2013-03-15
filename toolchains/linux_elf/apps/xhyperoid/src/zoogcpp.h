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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	ZoogDispatchTable_v1 *zdt;
	bool painted;
	ViewportID viewport;
	ZCanvas canvas;
	int cur_color;
#define PALETTE_SIZE 16
	uint32_t palette[PALETTE_SIZE];
} ZoogGraphics;

void zoog_init();
void zoog_verify_label(ZoogDispatchTable_v1 *zdt);
void zoog_paint_display();

int zoog_drawpixel(int x, int y);
void zoog_setcolor(int color);
int zoog_drawscansegment(unsigned char *colors, int x, int y, int length);

#ifdef __cplusplus
}
#endif
