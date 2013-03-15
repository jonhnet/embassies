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
#include "avl2.h"
#include "zutex_sync.h"
#include "LegacyZClock.h"
#include "ZLCEmit.h"

#include <time.h>	// CLOCK_REALTIME, CLOCK_MONOTONIC

class ZClock;

class ZTimer {
public:
	ZTimer(ZClock *zclock, uint64_t alarm_time);
	ZTimer(uint64_t alarm_time, uint32_t _tiebreaker = 0);	// key ctor
	~ZTimer();

	ZAS *get_zas();
	ZTimer getKey() { return ZTimer(alarm_time, _tiebreaker); }

	static inline int _cmp(ZTimer *za, ZTimer *zb)
	{
		if (za->alarm_time < zb->alarm_time) { return -1; }
		if (za->alarm_time > zb->alarm_time) { return +1; }
		// Break ties, which lets us insert the same time into the tree twice.
		return za->_tiebreaker - zb->_tiebreaker;
	}

	bool operator<(ZTimer b)
		{ return _cmp(this, &b) < 0; }
	bool operator>(ZTimer b)
		{ return _cmp(this, &b) > 0; }
	bool operator==(ZTimer b)
		{ return _cmp(this, &b) == 0; }
	bool operator<=(ZTimer b)
		{ return _cmp(this, &b) <= 0; }
	bool operator>=(ZTimer b)
		{ return _cmp(this, &b) >= 0; }

	bool check(ZutexWaitSpec *out_spec);
	uint64_t get_alarm_time() { return alarm_time; }

private:
	ZClock *zclock;
	uint64_t alarm_time;
	uint32_t _tiebreaker;
	bool fired;

	friend class ZClock;
	uint32_t private_zutex;
	ZutexWaitSpec spec;
		// whether we wait on private_zutex, or ZClock::alarm_zutex
		// depends on if we're the master or not.

	void deactivate();
	static bool s_check(ZoogDispatchTable_v1 *zdt, struct _s_zas *zas, ZutexWaitSpec *out_spec);
	static ZASMethods3 ztimer_zas_methods;

	struct s_zas_struct {
		ZAS zas;
		ZTimer *z_this;
	} zas_struct;
};

typedef AVLTree<ZTimer,ZTimer> ZTimerTree;

// This class multiplexes access to the single set_alarm Zoog service
// Invariant in this simple implementation:
// All waiters wait on the same zutex, and every time they check it,
// they consider updating the single Zoog timer to the next customer in
// line.
class ZClock {
public:
	ZClock(ZoogDispatchTable_v1 *zdt, SyncFactory *sf);
	~ZClock();

	uint64_t get_time_zoog();
	//void get_time_timespec(struct timespec *out_ts);

	LegacyZClock *get_legacy_zclock() { return &legacy.legacy_zclock; }
	ZLCEmit *get_ze() { return ze; }

	// ZTimer interface
	void activate(ZTimer *timer);
	void discharge_duties(ZTimer *timer);
	void deactivate(ZTimer *timer);
	uint32_t make_tiebreaker();

private:
	ZoogDispatchTable_v1 *zdt;
	uint32_t *alarm_zutex;
	uint32_t alarm_zutex_match_val;

	SyncFactoryMutex *mutex;
	ZTimer *master;
	ZTimerTree *timer_tree;

	ZLCEmit *ze;

	void _reset_host_alarm(uint64_t time);
	void _discharge_duties_nolock(ZTimer *timer);

	struct s_legacy {
		LegacyZClock legacy_zclock;
		ZClock *x_this;
	} legacy;

	static uint32_t s_tiebreaker;
	static LegacyZClockMethods legacy_zclock_methods;

	static LegacyZTimer *legacy_new_timer(LegacyZClock *zclock, const struct timespec *alarm_time_ts);
	static void legacy_read(LegacyZClock *zclock, struct timespec *out_cur_time);
	static ZAS *legacy_get_zas(LegacyZTimer *legacy_ztimer);
	static void legacy_free_timer(LegacyZTimer *legacy_ztimer);
};
