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
#ifndef _XPE_ROUTE_MACROS_H
#define _XPE_ROUTE_MACROS_H

#define ROUTE_PROLOGUE \
	XVFSHandleWrapper *wrapper = xpe->handle_table->lookup(filenum); \
	if (wrapper==NULL) { return -EBADF; } \
	XfsErr err = XFS_DIDNT_SET_ERROR;

#define ROUTE_AVAIL(zaschannel) \
	if ((wrapper->status_flags & O_NONBLOCK)!=0) \
	{ \
		ZAS *zas = wrapper->get_zas(zaschannel); \
		if (zas_check(xpe->zdt, &zas, 1)!=0) \
		{ \
			return -EAGAIN; \
		} \
	}

#define ROUTE_DECLARE_RC_VOID \
	int rc = 0;	/* if no error, return 0  */

#define ROUTE_DECLARE_RC_VALUE \
	int rc =	/* collect return value from xfs method call */
		
#define ROUTE_MAIN(FCN, ...) \
	wrapper->FCN(&err, __VA_ARGS__); \
	xax_assert(err!=XFS_DIDNT_SET_ERROR); \
	if (err!=XFS_NO_ERROR) \
	{ \
		return -err; \
	} \
	return rc;

#define ROUTE_VOID_FCN(FCN, ...) \
	ROUTE_PROLOGUE \
	ROUTE_DECLARE_RC_VOID \
	ROUTE_MAIN(FCN, __VA_ARGS__);

#define ROUTE_FCN(FCN, ...) \
	ROUTE_PROLOGUE \
	ROUTE_DECLARE_RC_VALUE \
	ROUTE_MAIN(FCN, __VA_ARGS__);

#define ROUTE_NONBLOCKING(FCN, zaschannel, ...) \
	ROUTE_PROLOGUE \
	ROUTE_AVAIL(zaschannel); \
	ROUTE_DECLARE_RC_VALUE \
	ROUTE_MAIN(FCN, __VA_ARGS__);

#endif // _XPE_ROUTE_MACROS_H
