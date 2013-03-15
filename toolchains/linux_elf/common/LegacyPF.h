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

/*
Weeelll, that is unfortunate. I want to have packetfilter MQEntries be C++
so they can carry ZeroCopyBufs, but one customer is lwipib/src/xaxif.c,
and it's C-only. I guess I need a wrapper class.
Ugh. I either need to fold some C++ code into lwip's compilation
chain (unpleasant), or I need a C wrapper interface -- but then that
thing has to be compiled in C++, which means it needs to live down below
the pseudosyscall interface. Yuck! Which is less trouble?
I'm going to bet on the latter.
*/

#include "zutex_sync.h"

typedef void LegacyMessage;
struct struct_legacy_pf;
typedef struct struct_legacy_pf LegacyPF;

typedef struct {
	ZAS *(*legacy_pf_get_zas)(LegacyPF *lpf);
	LegacyMessage *(*legacy_pf_pop)(LegacyPF *lpf);

	uint8_t *(*legacy_message_get_data)(LegacyMessage *lm);
	uint32_t (*legacy_message_get_length)(LegacyMessage *lm);
	void (*legacy_message_release)(LegacyMessage *lm);
} LegacyMethods;

struct struct_legacy_pf {
	LegacyMethods *methods;
};

#ifdef __cplusplus
#include "packetfilter.h"
LegacyPF *legacy_pf_init(PFHandler *pfh);
#endif // __cplusplus

