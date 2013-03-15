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

#include <time.h>		// struct timespec
#include "zutex_sync.h"	// ZAS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
} LegacyZTimer;

struct s_legacyzclock;
typedef struct s_legacyzclock LegacyZClock;

typedef struct {
	LegacyZTimer *(*new_timer)(LegacyZClock *zclock, const struct timespec *alarm_time_ts);
	void (*read)(LegacyZClock *zclock, struct timespec *out_cur_time);
	ZAS *(*get_zas)(LegacyZTimer *ztimer);
	void (*free_timer)(LegacyZTimer *ztimer);
} LegacyZClockMethods;

struct s_legacyzclock {
	LegacyZClockMethods *methods;
	// placebo struct; the back end contains a reference to the
	// new C++ ZClock object.
};

void ms_to_timespec(struct timespec *dst, uint32_t time_ms);
uint32_t timespec_to_ms(struct timespec *ts);
void timespec_add(struct timespec *dst, const struct timespec *t0, const struct timespec *t1);
void timespec_subtract(struct timespec *dst, const struct timespec *t0, const struct timespec *t1);

uint64_t zoog_time_from_timespec(const struct timespec *ts);
void zoog_time_to_timespec(uint64_t zoog_time, struct timespec *out_ts);

#ifdef __cplusplus
}
#endif
