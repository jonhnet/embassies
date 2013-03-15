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

class ZDBDatum {
public:
	ZDBDatum(uint8_t *data, uint32_t len, bool this_owns_data);
	ZDBDatum(const hash_t *hash);
	ZDBDatum(const char *str);
	~ZDBDatum();

	ZDBDatum();	// for counting marshalled loads
	ZDBDatum *alloc();
	void write_buf(const void *buf, int len);
	void write_str(const char *str);
	void write_uint32(uint32_t value);

	hash_t read_hash();
	int read_buf(void *buf, int capacity);
	void read_raw(void *buf, int capacity);
	char *read_str();
	uint32_t read_uint32();

	inline DBT *get_dbt() { return &dbt; }

	int cmp(ZDBDatum *other);
		// compares full values; ignores "file pointer"

private:
	void _init_alloc(int len);
	void _assert_no_overrun();
	ZDBDatum(int len);	// for writing loaded data

	DBT dbt;
	uint8_t *marshal_ptr;

private:
	bool this_owns_data;
};
