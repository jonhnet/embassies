#include <sys/time.h>
#include <string.h>
#include <malloc.h>

#define NUMBUFS	1
#define BUFSIZE	(140*1024*1024)

int main()
{
	char *buffer0 = malloc(BUFSIZE);
	char *buffer1 = malloc(BUFSIZE);
	struct timeval tv_start;
	gettimeofday(&tv_start, NULL);
	memcpy(buffer1, buffer0, BUFSIZE);
	struct timeval tv_end;
	gettimeofday(&tv_end, NULL);
	double sec = (tv_end.tv_sec - tv_start.tv_sec) + (tv_end.tv_usec - tv_start.tv_usec)/1000000.0;
	fprintf(stderr, "Elapsed time %f sec; %f Gbps\n", sec, BUFSIZE*8/sec / (1024*1024*1024));
	return 0;
}
