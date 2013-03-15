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

#include "ZCertChain.h"

class AppIdentity {
private:
	char identity_name[1000];	// internal name via zguest env var
	ZCertChain *certChain;
	bool hash_valid;
	hash_t pub_key_hash;

	void load_identity_name();
	void load_cert_chain();

public:
	AppIdentity();
	AppIdentity(const char *identity_name);
	~AppIdentity();

	const char *get_identity_name();
	ZCert *get_cert();
	ZCertChain *get_cert_chain();
	ZPubKey *get_pub_key();
	hash_t get_pub_key_hash();
	const char *get_domain_name();
};
