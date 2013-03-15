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

#ifdef __cplusplus
extern "C" {
#endif

#include "pal_abi/pal_types.h"

// formatting

#define NYBBLE_TO_HEX(n)	(((n)>9)?'a'+(n)-10:(n)+'0')

char *hexstr(char *buf, int val);
char *hexstr_08x(char *buf, int val);
char *hexstr_min(char *buf, int val, int min_digits);
void utoa(char *out, uint32_t in_val);
void dtoa(char *out, double val, int num_decimals);

// yet more conversion routines collected from all over the place.
// I'm pretty sure these are redundant with hexstr_min().
char *hexbyte(char *buf, uint8_t val);
char *hexlong(char *buf, uint32_t val);


// parsing

extern uint32_t str2hex(const char *buf);
extern int str2dec(const char *buf);


// I am SO TIRED of not having a printf available, because libc's has
// dependencies on arenas or TLS or whatever the heck is down there
// that exploads whenever I touch printf. So TODAY IT ENDS.

int cheesy_snprintf(char *s, int n, const char *fmt, ...);
	// s is always terminated.

#ifdef __cplusplus
}
#endif
