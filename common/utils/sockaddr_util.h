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

#include "xax_network_defs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void sockaddr_from_ep(struct sockaddr *sa, size_t sa_size, UDPEndpoint *ep);
void ep_from_sockaddr(UDPEndpoint *ep, struct sockaddr *sa, size_t sa_size);

#ifdef __cplusplus
}
#endif // __cplusplus

#define SOCKADDR_FROM_EP(sa, epp)	sockaddr_from_ep(&(sa), sizeof(sa), (epp));
#define EP_FROM_SOCKADDR(epp, sa)	ep_from_sockaddr((epp), &(sa), sizeof(sa));

