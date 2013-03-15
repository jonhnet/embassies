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

#include "GCanvasAcceptorIfc.h"
#include <zoog_monitor_session/zoog_monitor_session.h>

class GenodeCanvasAcceptor : public GCanvasAcceptorIfc {
private:
	ZoogMonitor::Session::AcceptCanvasReply *acr;

public:
	GenodeCanvasAcceptor(ZoogMonitor::Session::AcceptCanvasReply *acr);
	
	virtual void set_fb_dataspace(Genode::Dataspace_capability dcap);
	virtual ZCanvas *get_zoog_canvas();
};


