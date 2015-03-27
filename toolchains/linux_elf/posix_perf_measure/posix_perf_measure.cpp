#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "xe_mark_perf_point.h"

int main(int argc, const char** argv)
{
	if (argc<2)
	{
		fprintf(stderr, "Hey, no args!\n");
		exit(-1);
	}
	posix_perf_point_setup();
	execv(argv[1], (char*const*) argv+1);
}
