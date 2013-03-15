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
/*----------------------------------------------------------------------
| PAL_TYPES
|
| Purpose: Process Abstraction Layer Type Definitions
|
| Remarks: We want to avoid dependency on system includes (for compiling
|   against self-contained subsystems), while remaining compatible
|   with them.
|   Thus, we define those boring system types needed by the pal_abi here.
----------------------------------------------------------------------*/

#pragma once

#if ZOOG_NO_STANDARD_INCLUDES

#if !__bool_true_false_are_defined

#ifndef __cplusplus
#define bool	_Bool
#else
// bool built into C++
#endif
#define true 	1
#define false	0
#endif

//#ifndef __uint32_t_defined
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long long	uint64_t;
//# define __uint32_t_defined
//#endif

//#ifndef INT_MAX
#define INT_MAX __INT_MAX__
//#endif

//#if !defined(_STDDEF_H)
typedef unsigned int	size_t;
//#endif

#if !defined(NULL)
#define NULL (0)
#endif

#else // ZOOG_NO_STANDARD_INCLUDES

#include <limits.h>		// INT_MAX
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#ifndef _WIN32
#include <stdbool.h>
#endif // _WIN32

#endif // ZOOG_NO_STANDARD_INCLUDES

// UI types. Here because they're used in pal_extensions.h

typedef struct {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
} ZRectangle;

typedef uint32_t ViewportID;
	// A host-defined handle that's meaningful only within one application.

typedef uint64_t Deed;
	// One process may transfer its viewport (known by a local handle)
	// to another by way of a
	// Deed, a secret capability knowledge of which enables the
	// other process to accept the viewport (to a ViewportID handle
	// local to its own process.

typedef uint64_t DeedKey;
	// Each deed comes with random symmetric key material, enabling
	// the landlord and tenant to communicate anonymously with each other.


