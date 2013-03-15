/*
#include <stdio.h>
#include <alloca.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
*/

#include "LegacyZClock.h"

const uint64_t nano = 1000000000LL;

void ms_to_timespec(struct timespec *dst, uint32_t time_ms)
{
	dst->tv_sec = time_ms / 1000;
	uint32_t fracms = time_ms - dst->tv_sec*1000;
	dst->tv_nsec = fracms * 1000000;
}

uint32_t timespec_to_ms(struct timespec *ts)
{
	return (ts->tv_nsec / 1000000) + (ts->tv_sec * 1000);
}

void timespec_add(struct timespec *dst, const struct timespec *t0, const struct timespec *t1)
{
	dst->tv_sec = t0->tv_sec + t1->tv_sec;
	dst->tv_nsec = t0->tv_nsec + t1->tv_nsec;
	if (dst->tv_nsec > nano)
	{
		dst->tv_nsec -= nano;
		dst->tv_sec += 1;
	}
}

void timespec_subtract(struct timespec *dst, const struct timespec *t0, const struct timespec *t1)
{
	struct timespec c0 = *t0;
	struct timespec c1 = *t1;
	if (c0.tv_nsec < c1.tv_nsec)
	{
		c0.tv_nsec += nano;
		c0.tv_sec -= nano;
	}
	dst->tv_sec = c0.tv_sec - c1.tv_sec;
	dst->tv_nsec = c0.tv_nsec - c1.tv_nsec;
}

uint64_t zoog_time_from_timespec(const struct timespec *ts)
{
	return ts->tv_sec * nano + ts->tv_nsec;
}

void zoog_time_to_timespec(uint64_t zoog_time, struct timespec *out_ts)
{
	out_ts->tv_sec = zoog_time / nano;
	out_ts->tv_nsec = zoog_time - (out_ts->tv_sec * nano);
}
