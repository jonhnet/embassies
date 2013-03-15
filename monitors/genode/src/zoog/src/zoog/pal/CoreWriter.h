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

#include <base/stdint.h>

#include "ChannelWriter.h"

using namespace Genode;

class CoreWriter
{
	ChannelWriter *_cw;
	uint32_t dbg_size;
	uint32_t dbg_count;
	uint32_t dbg_ui_units;

public:
	CoreWriter(ChannelWriter *cw, uint32_t size);
	~CoreWriter();
	void set_dbg_size(uint32_t size);
	void write(void *ptr, int len);
	static bool static_write(void *v_core_writer, void *ptr, int len);
};
