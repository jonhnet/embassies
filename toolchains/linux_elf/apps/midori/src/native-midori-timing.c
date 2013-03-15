#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

void run_one()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t start_ns = tv.tv_sec * 1000000000LL + tv.tv_usec*1000LL;
	fprintf(stderr, "start %08x%08x\n", (uint32_t) (start_ns>>32), (uint32_t) (start_ns&0xffffffff));

	int pipes[2];
	int rc;
	rc = pipe(pipes);
	assert(rc==0);

	int pid = fork();
	if (pid==0) // child
	{
		rc = dup2(pipes[1], 2);
		perror("dup2");
		//fprintf(stderr, "dup2==%d", rc);
		assert(rc==2);
		execl("build/midori-pie", "build/midori-pie", NULL);
		assert(false);
	}

	FILE *fpipe = fdopen(pipes[0], "r");
	perror("fpipe");
	assert(fpipe!=NULL);
	while (true)
	{
		char buf[1000];
		char *p = fgets(buf, sizeof(buf), fpipe);
		if (p==NULL)
		{
			break;
		}
		fprintf(stderr, "read from child: '%s'\n", buf);
		const char *pattern = "MIDORI_LOAD_FINISHED %llx";
		uint64_t completion_time_ns;
		rc = sscanf(buf, pattern, &completion_time_ns);
		if (rc==1)
		{
			uint64_t elapsed_time_ns = completion_time_ns - start_ns;
			fprintf(stderr, "Elapsed time %d ms", elapsed_time_ns / 1000000);
		}
	}

	sleep(5);
}

int main()
{
	run_one();
	return 0;
}
