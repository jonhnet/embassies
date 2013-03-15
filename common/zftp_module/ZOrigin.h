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

#include "pal_abi/pal_types.h"

class ZOrigin {
public:
	virtual ~ZOrigin();
	virtual bool lookup_new(const char *url) = 0;
	virtual bool validate(const char *url, FreshnessSeal *seal) = 0;

	virtual const char *get_origin_name() = 0;
		// used to map persisted objects back to their ZOrigin factory.

	static uint32_t __hash__(const void *v_a);
	static int __cmp__(const void *v_a, const void *v_b);
};

class ZOriginKey : public ZOrigin {
public:
	ZOriginKey(const char *name);
	bool lookup_new(const char *url);
	bool validate(const char *url, FreshnessSeal *seal);
	const char *get_origin_name();

private:
	const char *name;
};
