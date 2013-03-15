#include <stdint.h>
#include <iostream>
#include <assert.h>

#include "timer.h"

#ifdef _WIN32
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#include <unistd.h> // For sleep()
#define Sleep(x) usleep(x*1000)
#endif // _WIN32

using namespace std;

double cpuFreq = 0;

uint64_t GetRDTSC() {
	//VS on x64 doesn't support inline assembly
   //__asm {
   //   ; Flush the pipeline
   //   XOR eax, eax
   //   CPUID
   //   ; Get RDTSC counter in edx:eax
   //   RDTSC
   //}
#ifdef _WIN32
	int CPUInfo[4];
	int InfoType = 0;
   __cpuid(CPUInfo, InfoType); // Serialize the pipeline
   return (unsigned __int64)__rdtsc();
#else
	uint64_t rv;
	__asm__ (
		"push		%%ebx;"
		"cpuid;"
		"pop		%%ebx;"
		:::"%eax","%ecx","%edx");
	__asm__ __volatile__ ("rdtsc" : "=A" (rv));
	return (rv);
#endif // _WIN32
}

void initTimers() {
	// Calibrate frequency
	uint64_t start = GetRDTSC();
	Sleep(200); // Sleep for 200ms
	uint64_t stop = GetRDTSC();
	
	cpuFreq = (stop - start) / (200 / 1000.0);

	cout << "Measured CPU at: " << cpuFreq << endl;
	assert(cpuFreq);
}

void startTimer(Timer* t) {
	assert(t);
	t->start = GetRDTSC();
}

void stopTimer(Timer* t) {
	assert(t);
	t->stop = GetRDTSC();
}

double getTimeSeconds(Timer* t) {
	assert(t);
	return (t->stop - t->start) / cpuFreq;
}

uint64_t getTimeCycles(Timer* t) {
	assert(t);
	return (t->stop - t->start);
}
