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

// This class represents the logical network link that all the apps live
// on. It isn't itself a NetIfc; instead, UnicastLink delivers via
// my list of apps, and BroadcastLink delivers to every app in my list.

#include "AppIfc.h"
#include "hash_table.h"
#include "SyncFactory.h"
#include "avl2.h"
#include "TunIDAllocator.h"

class AppLink
{
public:
	AppLink(SyncFactory *sf, MallocFactory *mf, TunIDAllocator* tunid);
	~AppLink();

	void connect(App *app);
	void disconnect(App *app);

	// accessors for UnicastLink & BroadcastLink
/*
	HashTable *get_app_ifc_table();
		// TODO should protect with mutex; BroadcastLink may be iterating
		// over it while someone else changes it. Gack.
*/
	void visit_every(ForeachFunc *ff, void *user_obj);
		// This visit needs some sort of sync, since it may
		// delete elements discovered while others are also lookup/visiting.
	AppIfc *lookup(AppIfc key);

	XIPAddr get_subnet(XIPVer ver);
	XIPAddr get_netmask(XIPVer ver);
	XIPAddr get_gateway(XIPVer ver);
	XIPAddr get_broadcast(XIPVer ver);

	void dbg_dump_table();

private:
	static void _dbg_dump_one(void *v_this, void *v_app_ifc);
	static void _tidy_one(void *v_this, void *v_app_ifc);
	void _tidy();

	XIPAddr subnet[2];
	XIPAddr netmask[2];
	XIPAddr gateway[2];

#if 0
	HashTable app_ifc_table;
	HashTable stale_ifcs;
		// when we want to delete an app_ifc, we put it in stale_ifcs to
		// schedule it for deletion, since this event occurs during a
		// hash_table_visit_every. Later, we come along and _tidy() up
		// the app_ifc_table by cleaning out everything from the stale_ifcs
		// set.
		// TODO you know, an AVL tree wouldn't have the concurrent-delete
		// limitation, saving all this hand-wringing.
#endif
	typedef AVLTree<AppIfc,AppIfc> AppIfcTree;
	AppIfcTree *app_ifc_tree;
};
