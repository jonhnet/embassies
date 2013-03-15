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

#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "cheesy_snprintf.h"

#if __cplusplus
#include "PerfMeasureIfc.h"

class PerfMeasure : public PerfMeasureIfc {
private:
	ZoogDispatchTable_v1 *zdt;
	debug_logfile_append_f *debug_logfile_append;

public:
	PerfMeasure(ZoogDispatchTable_v1 *zdt);
	void mark_time(const char *event_label);
};

extern "C" {
#endif // __cplusplus

// C-only interface
void perf_measure_mark_time(ZoogDispatchTable_v1 *zdt, const char *event_label);

#if __cplusplus
}
#endif // __cplusplus
