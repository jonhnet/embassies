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

#include "KeyVal.h"
#include "xax_network_defs.h"
#include "malloc_factory.h"
#include "SocketFactory.h"
#include "RandomSupply.h"
#include "KeyValClient.h"
#include "KeyDerivationKey.h"
#include "SymEncKey.h"
#include "ZLCEmit.h"

// Warning: Currently very copy happy

class EncryptedKeyValClient : public KeyValClient {
public:
	EncryptedKeyValClient(MallocFactory* mf, AbstractSocket* sock, UDPEndpoint* server, RandomSupply* random_supply, KeyDerivationKey* appKey, ZLCEmit *ze);
	~EncryptedKeyValClient();

	bool insert(KeyVal* kv);
	Value* lookup(Key* key);

private:
	SymEncKey* key;
	RandomSupply* random_supply;	
};

