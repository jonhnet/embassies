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

// common behavior between ZFSRFile and ZFSRDir

#include "ZFSReader.h"

class ZFSRInode : public ZFSReader {
public:
	ZFSRInode();
	virtual ~ZFSRInode();


	virtual bool get_filelen(uint32_t *filelen_out);
	virtual bool get_mtime(uint32_t *mtime_out);
	virtual bool get_unix_metadata(ZFTPUnixMetadata *out_zum);

protected:
	void fill_from_stat(struct stat *statbuf);

	uint32_t filelen;
	uint32_t mtime;
	ZFTPUnixMetadata zum;
};


