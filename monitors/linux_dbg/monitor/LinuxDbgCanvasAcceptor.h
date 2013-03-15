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

#include "XBlitCanvasAcceptorIfc.h"
#include "Process.h"
#include "linux_dbg_port_protocol.h"

class LinuxDbgCanvasAcceptor : public XBlitCanvasAcceptorIfc {
private:
	Process *process;
	XpReplyMapCanvas *reply;

	uint32_t framebuffer_size;
	uint8_t *framebuffer;

public:
	LinuxDbgCanvasAcceptor(Process *process, XpReplyMapCanvas *reply);
	virtual ZCanvas *make_canvas(uint32_t framebuffer_size);
	virtual uint8_t *get_framebuffer();
	virtual uint32_t get_framebuffer_size();
};

