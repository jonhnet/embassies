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

class SyncFactoryMutex
{
public:
	virtual ~SyncFactoryMutex() {};
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

class SyncFactoryEvent
{
public:
	virtual ~SyncFactoryEvent() {};
	virtual void wait() = 0;
	virtual bool wait(int timeout_ms) = 0;
	virtual void signal() = 0;
	virtual void reset() = 0;
};

class SyncFactory
{
public:
	virtual SyncFactoryMutex *new_mutex(bool recursive) = 0;
	virtual SyncFactoryEvent *new_event(bool auto_reset) = 0;
};

