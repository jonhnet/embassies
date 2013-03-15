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

#include <db.h>

#include "pal_abi/pal_types.h"
#include "ZFTPIndexConfig.h"
#include "zftp_protocol.h"
#include "ZDBDatum.h"

class ZDBCursor;

class ZDB {
public:
	ZDB(ZFTPIndexConfig *zic, const char *index_dir, const char *name);
		// open a database
	~ZDB();

	void insert(ZDBDatum *key, ZDBDatum *value);
	ZDBDatum *lookup(ZDBDatum *key);

	const char *get_name() { return filename; }

	int get_nkeys();

	ZDBCursor *create_cursor();

private:
	void lock();
	bool lock_inner();
	void unlock();
	bool check_config(ZFTPIndexConfig *zic);

	friend class ZDBCursor;
	DB *db;
	char *filename;
	char *lockfilename;
	char *pidfilename;
};
