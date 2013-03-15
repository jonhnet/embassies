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

#include "ZCache.h"
#include "ZLCArgs.h"
#include "standard_malloc_factory.h"
#include "SyncFactory.h"
#include "SocketFactory.h"
#include "ThreadFactory.h"
#include "SendBufferFactory.h"

class ZFTPApp {
public:
  void run(ZLCArgs *zlc_args, 
           MallocFactory *mf, 
           SocketFactory *socket_factory,
           SyncFactory *sf,           
           ThreadFactory *tf,
           SendBufferFactory *sbf);
	void close();

	ZCache *zcache;
};

