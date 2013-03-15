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

#include "LiteLib.h"

#ifdef _WIN32
#include <stdio.h>
#include <assert.h>
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif

inline void check(bool condition, const char *msg)
{
	if (!condition) {
#if ZLC_USE_PRINTF
		fprintf(stderr, "Args error: %s\n", msg);
		exit(-1);
#else
		lite_assert(false);
#endif // ZLC_USE_PRINTF
	}
}

inline int htoi(char *s)
{
	int v;
#ifdef _WIN32
	int rc = sscanf_s(s, "%x", &v);
#else
	int rc = sscanf(s, "%x", &v);
#endif // _WIN32
	lite_assert(rc==1);
	return v;
}

#define CONSUME_ARG(argname, field, decode)	{ \
	if (strcmp(argv[0], argname)==0) \
	{ \
		char buf[500]; snprintf(buf, sizeof(buf), "%s missing arg", argname); \
		check(argc>=2, buf); \
		char *value = argv[1]; \
		field = (decode); \
		argc-=2; argv+=2; continue; \
	} \
}

#define CONSUME_OPTION_ASSIGN(argname, signal, from_val, to_val)	{ \
	if (strcmp(argv[0], argname)==0) \
	{ \
		char errbuf[500]; \
		snprintf(errbuf, sizeof(errbuf), "option %s already assigns %s", \
			argname, #signal); \
		check((signal)==(from_val), errbuf); \
		signal = (to_val); \
		argc-=1; argv+=1; continue; \
	} \
}
#define CONSUME_OPTION(argname, signal) CONSUME_OPTION_ASSIGN(argname, signal, false, true)

#define CONSUME_BYFUNC(argname, func, userobj) { \
	if (strcmp(argv[0], argname)==0) \
	{ \
		char buf[500]; snprintf(buf, sizeof(buf), "%s missing arg", argname); \
		check(argc>=2, buf); \
		char *value = argv[1]; \
		func(userobj, value); \
		argc-=2; argv+=2; continue; \
	} \
}

#define CONSUME_STRING(a, f)	CONSUME_ARG(a, f, value)
#define CONSUME_BOOL(a, f)		CONSUME_ARG(a, f, strcmp(value, "true")==0)
#define CONSUME_INT(a, f)		CONSUME_ARG(a, f, atoi(value))
#define CONSUME_HEXINT(a, f)	CONSUME_ARG(a, f, htoi(value))

#define UNKNOWN_ARG()			{ char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]); check(false, buf); }

#ifdef __cplusplus
}
#endif
