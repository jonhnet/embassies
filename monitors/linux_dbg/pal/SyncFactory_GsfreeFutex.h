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

#include "SyncFactory_Zutex.h"

/*
Class: SyncFactory_GsfreeFutex 
Purpose: this synchronization implementation makes sense only in this goofball
linux_dbg pal context, where we're trying to synchronize between gsfree app threads
(where the zoog app controls the GS register, and hence we have to be very careful
not to call any system functions that futz with GS, even including implicit errno!)
and full-on pthreads threads. The gsfree users means we can't use SyncFactory_Pthreads;
and we can't use SyncFactory_Zutex, because that would have us calling back into
our own zutex implementation and out to the monitor -- what a mess!

Instead, we just use kernel futexes to directly implement the few cases of zutexes
that the SyncFactory_Zutex implementation requires. We recycle that implementation,
by passing it a special, fake-ish ZoogDispatchTable with only the two zutex functions
filled in.
*/

class SyncFactory_GsfreeFutex : public SyncFactory
{
public:
	SyncFactory_GsfreeFutex();
	virtual SyncFactoryMutex *new_mutex(bool recursive);
	virtual SyncFactoryEvent *new_event(bool auto_reset);

private:
	ZoogDispatchTable_v1 fake_zutex_dt;
	SyncFactory_Zutex *sf_zutex;

	static void zoog_zutex_wait(ZutexWaitSpec *specs, uint32_t count);
	static int zoog_zutex_wake(
		uint32_t *zutex,
		uint32_t match_val, 
		Zutex_count n_wake,
		Zutex_count n_requeue,
		uint32_t *requeue_zutex,
		uint32_t requeue_match_val);
};

