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

// A PAL passes an XDT into a loader (elf or pe or otherwise) explicitly
// in a register.
// But when elf_loader starts a proper Elf process, it doesn't have that
// particularly direct luxury. Instead, it has the luxury of argv, envp,
// and auxv. We (ElfArgs.c) pass the XDT into subprocesses via a made-up
// AUXV token, and we (get_xax_dispatch_table.c) extract the XDT by looking
// through the auxv for that token.

#define AT_XAX_DISPATCH_TABLE 0x01002009
