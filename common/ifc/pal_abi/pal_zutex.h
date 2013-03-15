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
/*----------------------------------------------------------------------
| PAL_ZUTEX
|
| Purpose: Process Abstraction Layer Zutex
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_types.h"

/*

A zutex is a mechanism for coordinating user-level memory synchronization
with monitor-level thread scheduling.

It's an attempt to find a minimal subset of
Linux' futex mechanism and Windows' user-kernel split synchronization
operations.

The overall idea is that a user-level thread uses processor/cache memory
synchronization to perform necessary locking on synchronization data
structures. If it finds that the data structure is in a state where this
thread must wait on another, the thread asks the monitor to put it on a
list named by an address relevant to the data structure it is blocked on,
and deschedule it.
When another thread moves the data structure into a state that should
release the first thread, it signals the monitor to release one or all of
the threads waiting on the named list.

The object ("zutex") over which the user-level code is waiting is named
by address; in the abstract, this is just a pedagogical tool: to the
monitor, the name is opaque; it need only connect "wakers" to "waiters".
For this, it hashes the zutex names.

In practice, a waiter must avoid the race condition. If it (a) reads the
value of the memory address, (b) decides that it needs to wait, and (c) asks
the monitor to queue it, another thread may arrive between (b) and (c),
update the object, and signal the queue (upon which the first thread hasn't
yet waited), leaving the first thread waiting indefinitely.
To avoid this, waiters want semantics that say "I just checked this zutex;
its value was X; I'd like to sleep assuming its value is still X"; this
makes the read atomic with the sleep. (Not exactly, but it's conservative
in the right direction -- the thread may be woken unecessarily.)

To that end, the monitor *is* allowed to understand that the zutex is actually
an address in user memory, so it can perform the verifying-read atomically
with the queue-thread operation. (For goofy monitors that can't see into the
app's address space, this atomicity can be achieved slowly by having the
PAL ask the monitor to lock the wait queue, re-check the zutex value,
then have the monitor unlock atomically with the enqueue operation.)

This basic idea has two extensions needed to cover cases commonly used by
real applications:
- A thread can wait on multiple zutexes, waking when the first zutex
is signaled.
- A thread can wake multiple zutexes (broadcast). To avoid convoys, it
can implement 'wake' by moving waiters from one zutex (waiting for a
condition) onto another (waiting for the lock needed to evaluate that
the condition is now ready).

Futex has evolved quite a bit since its introduction; zutex dispenses
with a few of its vestigial features.
Zutex also provides no support for a timeout argument; instead, the system
or app must create a zutex that's signaled at the desired timeout, and
the waiting thread simply includes another zutex in its wait list.
For a Linux futex overview, see:
Ulrich Drepper, "Futexes are tricky", http://www.akkadia.org/drepper/futex.pdf

A further clarification on wake semantics:

One can envision two intepretation of the semantics of wake(n):
(Strong) If wake(n) is called on a zutex on which q>n threads wait,
then the wake will cause n of those threads to transition from waiting
to runnable.
(Weak) If wake(n) is called on a zutex on which q>n threads
wait, then at the moment of wake, n of those threads will be runnable.

TODO what is the "correct" way?

Because we dont have an application scenario that requires the stronger
definition, we think the weaker one will do.

An example illustrating the difference:
There are two zutexes, z1 & z2.
Three threads are waiting: A(z1), B(z1,z2), C(z2).
If thread D wakes
z1 with n=1 and thread E wakes z2 with n=1, then:
- The weak definition (that we use) admits the possibility that B is the
only thread that transitions to runnable.
- The strong definition (that we don't use) would require two threads to
transition to runnable (can't double-count B).
*/

/*
Rationale:
Historically, ABIs often seem to start out with a blocking interface
(which makes writing single-threaded apps, such as the test case :v)
easy, but then later have difficulty generalizing to nonblocking and
asynchronous interfaces. POSIX is particularly awful about this;
select() only applies to some waitable object types; there's no good
way to get one thread to wait on both a file descriptor and a pthreads
data structure.
Windows gets it much better with WaitForMultipleObjects, but even
that interface is has its limitations: it returns when the slow call
has completed, not when it is ready to complete, making it difficult
to compose.

Zutexes, based on POSIX futexes, combine these insights to get really
minimal:
- The ubiquitous applicability of WaitForMultipleObjects. Zutexes are the
*only* way for a thread to idle in Zoog, so you can always combine any
two waitable objects into a single disjunctive wait.
- The minimalism of linux futexes: Zutexes do only *one* job: they
let a thread idle without introducing a race condition that might make
it miss its wake. Syncronization primitives (events, conditions) are
built out of them, and in Zoog, even poll/select on external events is
built out of them.

NB: while all calls in zoog are asynchronous, the goal is to simplify
the ABI (in particular, heading off the need for any fancier select()
sort of interface to clean up other blocking calls). Providing asynchronous
I/O for performance reasons (e.g. Windows' overlapped I/O) is a non-goal.
Apps can build their own overlapping by overlapping network messages,
but ultimately, that's a level of performance that seems more critical
for database servers than for UI boxes.
*/


/*----------------------------------------------------------------------
| Struct: ZutexWaitSpec
|
| Purpose: Zutex Wait Specification
----------------------------------------------------------------------*/
typedef struct {

    // Pointer to the zutex
    uint32_t *zutex;

    // TODO
    uint32_t match_val;

} ZutexWaitSpec;


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_zutex_wait
|
| Purpose: Wait on a Zutex
|
| This is the only blocking call in the Zoog Client Execution Interface.
| It's used to manage thread scheduling, with respect to both internal
| events (app-created zutexes signaled by other threads) and alarms
| (host-managed events). A thread may wait on the disjunction of any
| combination of app zutexes and host alarms.
| 
|
| Parameters: 
|   specs (IN) -- Zutex wait spec
|   count (IN) -- count to wait for
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (*zf_zoog_zutex_wait)(ZutexWaitSpec *specs, uint32_t count);


/*----------------------------------------------------------------------
| Define: ZUTEX_ALL
|
| Purpose: Used for selecting all Zutexes
----------------------------------------------------------------------*/
#define ZUTEX_ALL	INT_MAX


/*----------------------------------------------------------------------
| Typedef: Zutex_count
|
| Purpose: Zutex count type
----------------------------------------------------------------------*/
typedef int Zutex_count;


/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_zutex_wake
|
| Purpose: Wake on a zutex
|
| Remarks: with n_wake==ZUTEX_ALL, this subsumes FUTEX_WAKE,
|   and requeue parameters are irrelevant
|   with n_wake==1 and n_requeue==0, this subsumes FUTEX_WAKE,
|   and requeue parameters are irrelevant
|   With n_wake=={0,1} and n_requeue=={1,ZUTEX_ALL},
|   this subsumes FUTEX_CMP_REQUEUE.
|
| Parameters: 
|   zutex             (IN) -- Zutex
|   match_val         (IN) -- Match value
|   n_wake            (IN) -- Number to wake
|   n_requeue         (IN) -- Number to requeue
|   requeue_zutex     (IN) -- Requeue zutex
|   requeue_match_val (IN) -- Requeue match value
|
| Returns: The number of threads woken (greater than or equal to 0)
|	TODO ugh current implementations return -1 if n_requeue>0, to signal
|	an error and convince the overlying implementation to fall back to
|	non-requeue behavior. We should test the offending toolchain with
|	a naive implementation that wakes rather than requeues, so that we
|   don't have an error sentinel to return.
|   TODO it's not at all clear we actually need the requeue baloney.
|   We should consider boiling it back out of here for version 1.
|	TODO it's not clear we need a return value here. Try it without!
----------------------------------------------------------------------*/
typedef int (*zf_zoog_zutex_wake)(
    uint32_t *zutex,
    uint32_t match_val, 
    Zutex_count n_wake,
    Zutex_count n_requeue,
    uint32_t *requeue_zutex,
    uint32_t requeue_match_val);

/*----------------------------------------------------------------------
| Struct: ZoogHostAlarms
|
| The set of host-supplied alarms for host-generated events.
| An alarm is a host-signaled zutex.  Use these zutex to
| avoid polling the (non-blocking) interfaces.
|
| To signal each alarm, the host will increment *zutex and wake_zutex(n==1).
|
| A typical usage pattern uses zutex semantics to avoid a check-block race:
|
| while (1)
| {
|	uint32_t match_val = *alarm;
|	if (poll_nonblocking_interface() returns data) { break; }
|	zutex_wait({alarm, match_val});
| }
|
|
| The synchronization protocol for how the zutex is incremented is
| not declared; thus the app should only read, never update alarm zutex
| locations. (Hosts may in fact allocate them in pages that are read-only
| to the app.)
|
| NB the host isn't required to poke zutex X if the app hasn't called the
| corresponding X() interface since it was last poked.
| (where poke==increment and wake)
|
| Rationale:
| zutex_wait is the only mechanism for idling a thread in zoog; it applies
| both to waiting for a different app thread (which signals the zutex with
| zutex_wake), and to waiting for any host-provided events.
| An alternative design was to allow the application to register zutex
| addresses with various host services, but that led to unneeded complexity.
| To keep things minimal, the host declares a set of zutexes that it
| promises to poke whenever a corresponding interface goes from an 'empty'
| to a 'ready' state.
----------------------------------------------------------------------*/
typedef struct
{
	// use instead of polling get_time()
	// see set_clock_alarm
	uint32_t clock;

	// use instead of polling receive_net_buffer()
	uint32_t receive_net_buffer;

	// use instead of polling receive_ui_event()
	uint32_t receive_ui_event;

#if 0 // Future
	// use instead of polling receive_fault_event()
	uint32_t fault;
#endif
} ZoogHostAlarms;

// These need to stay in sync as indices of zutexes in ZoogHostAlarms;
// they provide an array-index mechanism to reach into that data structure,
// so we can write general-purpose code that is parameterized by alarm id.
enum AlarmEnum {
	alarm_clock = 0,
	alarm_receive_net_buffer,
	alarm_receive_ui_event,
	alarm_count
};

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_get_alarms
|
| Purpose: Retrieve the address of the host-signaled alarm zutexes
|
| Parameters: none
|
| Returns: Pointer to the received network buffer
----------------------------------------------------------------------*/
typedef ZoogHostAlarms *(*zf_zoog_get_alarms)();
