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

#include <dataspace/capability.h>
#include "BlitPalIfc.h"

// common/BlitCanvas asks BlitProviderIfc to create a canvas implementation.
// In turn, BlitProviderIfc is going to ask the specific monitor how to
// make the shm segment it needs. That's this ifc.

class GCanvasAcceptorIfc
	: public BlitCanvasAcceptorIfc {
public:
	virtual void set_fb_dataspace(Genode::Dataspace_capability dcap) = 0;
	virtual ZCanvas *get_zoog_canvas() = 0;
};


