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

#include "pal_abi/pal_basic_types.h"
#include "LiteAssert.h"

#ifdef __cplusplus
extern "C" {
#endif

void lite_strcpy(char *dst, const char *src);
void lite_strncpy(char *dst, const char *src, int n);
size_t lite_strlen(const char *s);
size_t lite_strnlen(const char *s, size_t max);
const char *lite_strchrnul(const char *p, char c);
void lite_strcat(char *dst, const char *src);
void lite_strncat(char *dst, const char *src, int max);
int lite_strcmp(const char *a, const char *b);
char *lite_index(const char *s, int c);
char *lite_rindex(const char *s, int c);
bool lite_starts_with(const char *prefix, const char *str);
bool lite_ends_with(const char *suffix, const char *str);

void lite_memcpy(void *dst, const void *src, size_t n);
void lite_memset(void *start, char val, int size);
void lite_bzero(void *start, int size);
int lite_memcmp(const void *s1, const void *s2, size_t n);
void *lite_memchr(const void *start, char val, size_t n);
bool lite_memequal(const char *start, char val, int size);
	// return true if every byte of start[0..size-1] has value val
uint32_t lite_atoi(const char *s);

#ifdef __cplusplus
}
#endif
