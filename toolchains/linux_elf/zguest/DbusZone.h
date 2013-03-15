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

#include "Zone.h"

class DbusZone : public Zone
{
public:
	DbusZone();

	char *get_bus_address();
	void polymorphic_break();

private:
	char argbuf[20];
	int pipes[2];
};

class DbusClientZone : public Zone
{
public:
	DbusClientZone(
		DbusZone *dbus,
		const char *label, int libc_brk_heap_mb, const char *app_path);
	
	// override to stuff in dbus env vars
	virtual void synchronous_setup(LaunchState *ls);

private:
	DbusZone *dbus;
};

