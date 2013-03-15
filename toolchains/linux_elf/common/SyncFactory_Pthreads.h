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
#include <pthread.h>
#include "SyncFactory.h"

class SyncFactoryMutex_Pthreads : public SyncFactoryMutex
{
public:
	virtual ~SyncFactoryMutex_Pthreads();
	virtual void lock();
	virtual void unlock();

private:
	friend class SyncFactory_Pthreads;
	SyncFactoryMutex_Pthreads(bool recursive);

	pthread_mutex_t mutex;
};

class SyncFactoryEvent_Pthreads : public SyncFactoryEvent
{
public:
	virtual ~SyncFactoryEvent_Pthreads();
	virtual void wait();
	virtual bool wait(int timeout_ms);
	virtual void signal();
	virtual void reset();

private:
	friend class SyncFactory_Pthreads;
	SyncFactoryEvent_Pthreads(bool auto_reset);

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool auto_reset;
	bool event_is_set;
};

class SyncFactory_Pthreads : public SyncFactory
{
public:
	SyncFactory_Pthreads();
	virtual SyncFactoryMutex *new_mutex(bool recursive);
	virtual SyncFactoryEvent *new_event(bool auto_reset);

private:
};


