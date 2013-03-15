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
#include "xvnc_start_detector.h"
#include "Zone.h"

class RunConfig
{
public:
	RunConfig(MallocFactory *mf, const char **incoming_envp);

	void add_zone(Zone *zone);
	void add_vnc();
	Zone *add_app(const char *appname);
//	Zone *add_zone(const char *label, int libc_brk_heap_mb, const char *repos_path);

	const char **get_incoming_envp() { return incoming_envp; }
	XvncStartDetector *get_xvnc_start_detector() { return &xvnc_start_detector; }
	LinkedList *get_zones() { return &zones; }

	static char *map_pie(const char *appname);
	
private:
	LinkedList zones;
	const char **incoming_envp;
	XvncStartDetector xvnc_start_detector;
};

// Each *.config.c defines this function:
void config_create(RunConfig *rc);
