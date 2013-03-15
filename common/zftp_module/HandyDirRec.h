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

#include <string.h>

#include "zftp_dir_format.h"
#include "LiteLib.h"
#include "malloc_factory.h"

class HandyDirRec {
private:
	MallocFactory *mf;
	ZFTPDirectoryRecord *drec;
	char *name;

public:
	HandyDirRec(MallocFactory *mf, ZFTPDirectoryRecord *drec, const char *name);

	HandyDirRec(const char *name);	// key ctor

	~HandyDirRec();

	HandyDirRec getKey() {
		return HandyDirRec(name);
	}

	const char *get_filename() { return name; }
	uint32_t get_record_size() {
		return sizeof(ZFTPDirectoryRecord) + lite_strlen(name) + 1; }
	void write(void *dest);

	bool operator<(HandyDirRec b)
		{ return strcmp(get_filename(), b.get_filename())<0; }
	bool operator>(HandyDirRec b)
		{ return strcmp(get_filename(), b.get_filename())>0; }
	bool operator==(HandyDirRec b)
		{ return strcmp(get_filename(), b.get_filename())==0; }
	bool operator<=(HandyDirRec b)
		{ return strcmp(get_filename(), b.get_filename())<=0; }
	bool operator>=(HandyDirRec b)
		{ return strcmp(get_filename(), b.get_filename())>=0; }
};

