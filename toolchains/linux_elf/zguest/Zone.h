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

#include "linked_list.h"
#include "hash_table.h"
#include "LaunchState.h"
#include "DetectorIfc.h"

#define VNC_PORT_NUMBER 6

class RunConfig;

class Zone
{
private:
	// configured state
	const char *label;
	int libc_brk_heap_mb;

	const char *app_path;
	LinkedList app_args;
	HashTable app_envs;

	DetectorIfc *detector_ifc;
		// !=NULL if this zone should wait for something
		// before it starts running.

	// derived state
	char _ldso_brk_name[80];
	char _libc_brk_name[80];

	// runtime state
	LaunchState *_ls;
	pthread_t thread;	// when run_on_pthread

	bool break_before_launch;

	void _insert_default_env(const char *env);
	static void _install_env_in_eb(void *v_eb, void *v_datum);
	static void *_run_stub(void *v_this);

protected:
	ZoogDispatchTable_v1 *get_zdt();

public:
	Zone(const char *label, int libc_brk_heap_mb, const char *app_path);
	virtual ~Zone() {}	// not that we'll ever call it.
	virtual void run();
	void run_on_pthread();

	void add_arg(const char *arg);
	void add_env(const char *env);
	void prepend_env(RunConfig *rc, const char *name, const char *val);
	void set_detector(DetectorIfc *detector_ifc);

	virtual void synchronous_setup(LaunchState *ls);

	void set_break_before_launch(bool break_before_launch);
	virtual void polymorphic_break();
};

