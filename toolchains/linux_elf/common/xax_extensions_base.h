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

#define _xax_fake_syscall_base	900
#define __NR_xe_get_system_clock			(_xax_fake_syscall_base+0x01)
#if 0
#define __NR_xe_allocate_port_number		(_xax_fake_syscall_base+0x02)
#endif
#define __NR_xe_network_add_default_handler	(_xax_fake_syscall_base+0x03)
#define __NR_xe_xpe_mount_vfs				(_xax_fake_syscall_base+0x04)
#define __NR_xe_allow_new_brk				(_xax_fake_syscall_base+0x05)
#define __NR_xe_mount_fuse_client			(_xax_fake_syscall_base+0x06)
#define	__NR_xe_wait_until_all_brks_claimed	(_xax_fake_syscall_base+0x07)
#define __NR_xe_register_exit_zutex			(_xax_fake_syscall_base+0x08)
#define __NR_xe_get_dispatch_table			(_xax_fake_syscall_base+0x09)
#define __NR_xe_open_zutex_as_fd			(_xax_fake_syscall_base+0x0a)
#define __NR_xe_mark_perf_point				(_xax_fake_syscall_base+0x0b)

