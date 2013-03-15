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

#include "RandomSupply.h"
#include "ZKeyPair.h"
#include "KeyDerivationKey.h"

// Common crypto machinery needed across monitors

class MonitorCrypto
{
private:
	// Master pool of randomness
	RandomSupply* random_supply;

	// Root of the Zoog key hierarchy.  Used to verify labels
	ZPubKey*  zoog_ca_root_key;

	// Used to derive app-specific secrets
	KeyDerivationKey* app_master_key;

	// Keypair for _this_ monitor.  Used to endorse local app keypairs
	ZKeyPair* monitorKeyPair;

public:
	enum KeyPairRequest { WANT_MONITOR_KEY_PAIR, NO_MONITOR_KEY_PAIR };
	MonitorCrypto(KeyPairRequest kpr);

	RandomSupply* get_random_supply() { return random_supply; }
	ZPubKey* get_zoog_ca_root_key() { return zoog_ca_root_key; }
	KeyDerivationKey* get_app_master_key() { return app_master_key; }
	ZKeyPair* get_monitorKeyPair() { return monitorKeyPair; }
	void set_monitorKeyPair(ZKeyPair* pr) { monitorKeyPair = pr; }
};
