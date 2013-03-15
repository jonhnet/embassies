// posix variation includes
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "xe_mark_perf_point.h"

const char *posix_filename = "debug_perf_posix";

void posix_mark_perf_point(char *descr);

void posix_perf_point_setup()
{
	unlink(posix_filename);
	posix_mark_perf_point("start");
}

void posix_mark_perf_point(char *descr)
{
	struct timeval tv;
	int rc = gettimeofday(&tv, NULL);
	assert(rc==0);
	uint64_t time_ns =
		((uint64_t) tv.tv_sec)*1000000000 + ((uint64_t) tv.tv_usec)*1000;
	FILE *fp = fopen(posix_filename, "a");
	fprintf(fp, "mark_time %llX %s\n", time_ns, descr);
	fclose(fp);
}

void xe_mark_perf_point(char *descr)
{
	int rc = syscall(__NR_xe_mark_perf_point, descr);
	if (rc!=0)
	{
		// Oh, zoog isn't below us. Emit elsewhere.
		posix_mark_perf_point(descr);
	}
}
