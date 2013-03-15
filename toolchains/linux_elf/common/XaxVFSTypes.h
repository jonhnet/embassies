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

typedef enum x_xfs_err {
	XFS_NO_ERROR = 0,
	XFS_SILENT_FAIL = 333,
	XFS_DIDNT_SET_ERROR = -6,
	XFS_ERR_USE_FSTAT64 = -7,
	XFS_NO_INFO = -8,
		// returned by mount overlays that have no information,
		// positive or negative, about the open.
} XfsErr;

typedef struct s_xfs_path {
	char *pathstr;
	char *suffix;	// part after the mount match
} XfsPath;

struct xi_kernel_dirent64
{
	uint64_t			d_ino;
	int64_t				d_off;
	unsigned short int	d_reclen;
	unsigned char		d_type;
	char				d_name[256];
};


