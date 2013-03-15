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

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_get_time
|
| Purpose:  Learn the current value of the real-time clock
|
| Parameters: 
|   out_time      (IN) -- Filled on return with a 64-bit integer
|					giving the system's notion of real-time, in nanoseconds.
|
| "Real time" is based on a local oscillator, with no guarantee of
| connection to reality. The nanoseconds will ideally run at a fairly
| consistent rate (modulo oscillator drift), and a rate close to real
| nanoseconds. The base is undefined, although it should not begin near
| 0xff*, so that apps need not be concerned about the clock wrapping.
| (2^64 nanoseconds is something like 584 years.)
|
| Returns: none
----------------------------------------------------------------------*/

typedef void (*zf_zoog_get_time)(uint64_t *out_time);

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_set_clock_alarm
|
| Purpose: Set a real-time alarm to signal a zutex.
|
| Remarks: 
|  At such time as zf_zoog_get_time() reads greater than the
|  scheduled_time argument, the ZoogHostAlarms.clock alarm zutex
|  is signaled.
|
|  If the specified scheduled_time has already elapsed, then the alarm
|  will occur right away (that is, won't be dropped).
|
|  Alarm semantics are pretty floppy: Obviously, we can't guarantee
|  that your alarm will arrive exactly on time. On the other hand,
|  the monitor might also signal alarms too early; in that case,
|  check the time and reset the alarm. (If this happens way too often,
|  of course, it's a performance issue.)
|
|  There is only one alarm object per application; a call to set_clock_alarm()
|  that occurs before a previously-set alarm has expired will cancel
|  and replace the previous alarm.
|
|  If zutex==NULL, no alarm is set, and any previous alarm is canceled.
|
|  Note that there will always eventually be a signalling of the zutex
|  after each call to zoog_set_clock_alarm. In particular, just because
|  the time is already expired, or the proposed time equals a prior
|  setting, it's not okay for the monitor to "absorb" the request.
|  The caller will have read the alarm zutex value before this call,
|  and will be relying on an eventual zutex wake for liveness.
----------------------------------------------------------------------*/
typedef void (*zf_zoog_set_clock_alarm)(uint64_t scheduled_time);
