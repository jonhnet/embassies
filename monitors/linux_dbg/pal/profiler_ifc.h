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

#include <stdint.h>

void _profiler_assert(int cond);

typedef int (profiler_thread_func)(void *arg);
struct timespec;

int _profiler_open(const char* filename, int flags, int mode);
void _profiler_write(int fd, const void *buf, int len);
void _profiler_close(int fd);
void _profiler_nanosleep(struct timespec *req);
int profiler_thread_create(profiler_thread_func *func, void *arg, uint32_t stack_size);
	// return the id of the created thread
	// May not be called more than once.
int profiler_my_id();
	// return the id of this thread, to see if it's the one from profiler_thread_create
