#include <stdio.h>
#include "xvnc_start_detector.h"

XvncStartDetector::XvncStartDetector()
{
	this->available = false;
	pthread_mutex_init(&this->vnc_start_mutex, NULL);
	pthread_cond_init(&this->vnc_start_cond, NULL);
}

void XvncStartDetector::wait(ZoogDispatchTable_v1 *xdt)
{
	fprintf(stderr, "vncPlus waiting for vnc_start_cond\n");
	pthread_mutex_lock(&vnc_start_mutex);
	while (!available)
	{
		pthread_cond_wait(&vnc_start_cond, &vnc_start_mutex);
	}
	pthread_mutex_unlock(&vnc_start_mutex);
	fprintf(stderr, "vncPlus got vnc_start_cond, proceeding!\n");
}

void XvncStartDetector::signal()
{
	fprintf(stderr, "synchronize_vnc_start called. Yay!\n");
	pthread_mutex_lock(&vnc_start_mutex);
	available = true;
	pthread_cond_signal(&vnc_start_cond);
	pthread_mutex_unlock(&vnc_start_mutex);
}
