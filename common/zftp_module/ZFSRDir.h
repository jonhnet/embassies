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

#include "ZFSRInode.h"
#include "zftp_dir_format.h"
#include "HandyDirRec.h"

class ZFSRDir : public ZFSRInode {
public:
	ZFSRDir();
	bool open();
		// Bryan: this interface changed. ZFSRDir_Win should accept its
		// path in its constructor, stow it away, and then use it when
		// it sees the virtual scan() call come down.

	virtual ~ZFSRDir();
	virtual bool is_dir(bool *isdir_out);
	virtual uint32_t get_data(uint32_t offset, uint8_t *buf, uint32_t size);
	virtual bool is_cacheable();

protected:
	void scan_add_one(HandyDirRec *hdr);

	virtual bool fetch_stat() = 0;

	// A backend-OS-specific subclass should implement 'scan'
	// by opening the directory, calling scan_add_one once for
	// each entry, and closing the directory afterwards.
	// NB this function will be called twice, once to count up
	// how much space the entries take, and a second time
	// to actually read them into the allocated space.
	virtual bool scan() = 0;

private:
	uint8_t *precomputed_contents;

	// values used during scan procedure
	uint32_t scan_len;
	uint32_t precomputed_contents_len;

};

