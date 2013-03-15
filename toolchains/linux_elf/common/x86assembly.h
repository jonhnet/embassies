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
#ifndef _X86ASSEMBLY_H
#define _X86ASSEMBLY_H

// These have to be declared static, because we take their addresses,
// which makes them get allocated. If they're externally visible,
// we get link conflicts.
// This makes multiple copies, but it lets us use them from multiple
// files in the same binary.
static const uint8_t x86_jmp_relative[1] = { 0xe9 };
static const uint8_t x86_nop = 0x90;
static const uint8_t x86_hlt = 0xf4;
static const uint8_t x86_call = 0xe8;
static const uint8_t x86_int80[] = {0xcd, 0x80};
static const uint8_t x86_ret = {0xc3};
static const uint8_t x86_push = {0x06};
static const uint8_t x86_push_eax = {0x50};
static const uint8_t x86_push_ebx = {0x53};
static const uint8_t x86_pop_eax = {0x58};
static const uint8_t x86_pop_ebx = {0x5b};
static const uint8_t x86_pop_ecx = {0x59};
static const uint8_t x86_mov_deref_ebx_to_eax[] = {0x8b, 0x43};
static const uint8_t x86_add_deref_ebx_to_eax[] = {0x03, 0x43};
static const uint8_t x86_add_immediate_eax[] = {0x05};
static const uint8_t x86_sub_immediate_eax[] = {0x2b, 0x05};
static const uint8_t x86_call_indirect_eax[] = {0xff, 0xd0};
static const uint8_t x86_mov_eax_to_esp[] = {0x89, 0xc4};
static const uint8_t x86_mov_esp_to_ebp[] = {0x89, 0xe5};
static const uint8_t x86_movl_immediate_to_ecx = {0xb9};
static const uint8_t x86_push_immediate = {0x68};
static const int trampoline_patch_size = sizeof(x86_jmp_relative)+sizeof(uint32_t);
static const int return_patch_size = sizeof(x86_jmp_relative)+sizeof(uint32_t);
static const int call_dispatcher_size = sizeof(x86_call)+sizeof(uint32_t);

#endif // _X86ASSEMBLY_H
