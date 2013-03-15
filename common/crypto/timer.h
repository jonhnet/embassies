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
#ifndef _TIMER_H
#define _TIMER_H

#ifdef _WIN32
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <Windows.h>
#endif // _WIN32

typedef unsigned long long uint64_t;

typedef struct s_timer {
	uint64_t start;
	uint64_t stop;
} Timer;

void initTimers();
void startTimer(Timer* t);
void stopTimer(Timer* t);
double getTimeSeconds(Timer* t);
uint64_t getTimeCycles(Timer* t);

#endif // _TIMER_H