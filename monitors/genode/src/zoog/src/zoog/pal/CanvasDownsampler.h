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

#include "hash_table.h"
#include "pal_abi/pal_ui.h"
#include "nitpicker_gfx/pixel_rgb565.h"

//typedef Pixel_rgb<int32_t, 0xff0000, 16, 0x00ff00, 8, 0x00ff, 0> Pixel_zoog_true24;

class Pixel_zoog_true24 {
private:
	uint32_t pixel;
public:
	inline uint32_t r() { return (pixel>>16) & 0x0ff; }
	inline uint32_t g() { return (pixel>> 8) & 0x0ff; }
	inline uint32_t b() { return (pixel    ) & 0x0ff; }
};

class CanvasDownsampler
{
private:
	ZCanvasID canvas_id;
	ZCanvas *canvas;
	MallocFactory *mf;
	Pixel_zoog_true24 *app_canvas;
//	Chunky_canvas<Pixel_rgb565> *np_canvas;
	Pixel_rgb565 *np_canvas;

public:
	CanvasDownsampler(ZCanvas *canvas, uint8_t *np_bits, MallocFactory *mf);
	CanvasDownsampler(ZCanvasID canvas_id);	//key
	~CanvasDownsampler();

	void update(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

class CanvasDownsamplerManager
{
private:
	MallocFactory *mf;
	HashTable ht;

public:
	CanvasDownsamplerManager(MallocFactory *mf);
	~CanvasDownsamplerManager();

	void create(ZCanvas *canvas, uint8_t *np_bits);
	CanvasDownsampler *lookup(ZCanvasID canvas_id);
};
