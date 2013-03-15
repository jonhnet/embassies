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
#include <string.h>
#include <malloc.h>
#include "NavIfcs.h"
#include "LiteLib.h"

class UrlEP : public EntryPointIfc {
private:
	char *url;
public:
	UrlEP(const char *_url) : url(strdup(_url)) {}
	~UrlEP() { free(url); }
	virtual void serialize(NavigationBlob* blob)
	{
		blob->append(url, strlen(url)+1);
	}
	const char* repr() { return url; }
	const char* get_url() { return url; }
};

class UrlEPFactory : public EntryPointFactoryIfc {
public:
	virtual EntryPointIfc* deserialize(NavigationBlob* blob)
	{
		uint32_t ep_size = blob->size();
		char *_url = (char*) blob->read_in_place(ep_size);
		lite_assert(_url[ep_size-1] == '\0');
		return new UrlEP(_url);
	}
};

