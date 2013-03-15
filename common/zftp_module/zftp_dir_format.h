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

// These values should synchronize with definitions in posix's dirent.h
typedef enum {
    ZT_UNKNOWN = 0,
    ZT_DIR = 4,		// entry refers to a directory
    ZT_REG = 8,		// entry refers to a regular file
} ZDirectoryRecordType;

typedef struct {
	uint32_t reclen;
		// size of this entire ZFTPDirectoryRecord. Next record begins
		// reclen bytes after this one began.
	uint32_t type;	 // enum as defined in ZDirectoryRecordType
	uint64_t inode_number;
		// jonh leaving this as cargo cult -- some posix files seem to depend
		// on telling files apart by their inode numbers.
		// For Windows, well, make something up. Be pretty cool if it were
		// sort of stable, or somehow agreed with the stat() value
		// for the file.
//	char filename[0];
		// zero-terminated filename
} ZFTPDirectoryRecord;

// workaround for missing 'filename[0]' declaration in VS
inline static char *ZFTPDirectoryRecord_filename(ZFTPDirectoryRecord *rec)
{ return (char*) &rec[1]; }
