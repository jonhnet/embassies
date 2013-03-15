#include <stdio.h>
#include <assert.h>
#include "GTimer.h"

void GTimer::start()
{
	gettimeofday(&t0, NULL);
}

void GTimer::stop()
{
	gettimeofday(&t1, NULL);
}

double GTimer::getTimeSeconds()
{
	double esec = t1.tv_sec - t0.tv_sec;
	double eusec = (t1.tv_usec - t0.tv_usec) / 1000000.0;
	assert(eusec > -1.0 && eusec < 1.0);
	double total = esec+eusec;
	return total;
}
