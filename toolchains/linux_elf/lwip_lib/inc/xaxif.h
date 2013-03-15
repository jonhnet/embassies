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
#ifndef __XAXIF_H__
#define __XAXIF_H__

#include "lwip/netif.h"
#include "lwip/api.h"
#include "zutex_sync.h"

void xaxif_setup(void *zdt);
err_t xaxif_init(struct netif *netif);
ZAS *lwip_get_read_zas(int sockfd);
void xax_arch_sem_configure(ZoogDispatchTable_v1 *zdt);

struct netconn *get_netconn_for_socket(int s);
	// hook defined in sockets.c patch

ZAS *sys_arch_get_mbox_zas(struct netconn *nconn);
	// hook defined in unix_arch.c patch

#endif /* __XAXIF_H__ */

