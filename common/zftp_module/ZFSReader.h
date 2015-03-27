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
#include "malloc_factory.h"
#include "zftp_unix_metadata_format.h"
#include "WriterIfc.h"

class ZFSReader {
public:
	virtual ~ZFSReader();
	virtual bool get_filelen(uint32_t *filelen_out) = 0;
	virtual bool is_dir(bool *isdir_out) = 0;
	virtual uint32_t get_data(uint32_t offset, uint8_t *buf, uint32_t size) = 0;
	virtual bool get_mtime(uint32_t *mtime_out) = 0;
	virtual bool get_unix_metadata(ZFTPUnixMetadata *out_zum) = 0;
	virtual bool is_cacheable() = 0;

	void copy(WriterIfc* writer, uint32_t offset, uint32_t len);
};
