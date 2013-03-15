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

#include "xax_extensions_base.h"

#include "LegacyZClock.h"
#include "LegacyPF.h"

#ifdef __cplusplus
extern "C" {
#endif

LegacyZClock *xe_get_system_clock(void);

#if 0
uint16_t xe_allocate_port_number(void);
#endif

LegacyPF *xe_network_add_default_handler();

int xe_xpe_mount_vfs(const char *prefix, void /*XaxFileSystem*/ *xfs);

void xe_allow_new_brk(uint32_t size, const char *dbg_desc);

/*
void xe_mount_fuse_client(const char *mount_point, consume_method_f *consume_method, ZQueue **out_queue, ZoogDispatchTable_v1 **out_xdt);
*/

void xe_wait_until_all_brks_claimed(void);

void xe_register_exit_zutex(uint32_t *zutex);

ZoogDispatchTable_v1 *xe_get_dispatch_table();
	// Used with caution! Many ZDT services are one-instance-only;
	// if you try to start fetching network packets, you'll conflict
	// with the XPE packet handler. (That's why we have
	// xe_network_add_default_handler above, which is multiplexed by
	// XPE.)
	// I added this interface to let XVnc take ownership of the
	// blit interface.

int xe_open_zutex_as_fd(uint32_t *zutex);
	// Provides an fd that refers to a zutex, so you can select/poll on
	// the zutex along with other fds. Reads and writes on this fd will
	// assert or maybe return an error; I haven't decided yet. Whimsy!

#ifdef __cplusplus
}
#endif
