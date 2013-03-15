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

#include "UIEventDeliveryIfc.h"
#include "ZPubKey.h"

// This interface is how an implementation specifies its wiring to its
// PALs. Each BlitPalIfc is one Zoog application.
// In linux_dbg, for example, a PalIfc is a Process, the object representing
// a live PAL process.

class BlitPalIfc {
public:
	virtual UIEventDeliveryIfc *get_uidelivery() = 0;
	virtual ZPubKey *get_pub_key() = 0;
	virtual void debug_remark(const char *msg) = 0;
};

// A void* interface (sorry, not sure how to improve that) that lets
// provider be generic wrt specific monitors. This is a "user object"
// that lets the monitor carry along info about how to connect the
// canvas to a specific PAL.
class BlitCanvasAcceptorIfc {
};
