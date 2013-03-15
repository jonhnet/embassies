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
#ifndef _XZFTP_XAX_NETWORK_STATE_H
#define _XZFTP_XAX_NETWORK_STATE_H

#include "pal_abi/pal_abi.h"
#include "zftp_protocol.h"
#include "zftp_network_state.h"
#include "xax_posix_emulation.h"

typedef struct {
	ZFTPNetworkMethods methods;

	MallocFactory *mf;
	XaxSkinnySocket skinny_socket;
	UDPEndpoint remote_ep;
} ZFTPNetXax;

ZFTPNetworkState *xzftp_xax_network_state_new(XaxPosixEmulation *xpe);

#endif // _XZFTP_XAX_NETWORK_STATE_H

