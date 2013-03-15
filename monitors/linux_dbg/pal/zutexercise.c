#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "pal_abi/pal_abi.h"
#include "xaxInvokeLinux.h"
#include "linux_dbg_port_protocol.h"
#include "zutexercise.h"
#include "zoog_qsort.h"

#define NUM_THREADS 4
#define NUM_ZUTEXES 4

typedef struct {
	XaxDispatchTable_v1 *dispatch_table;
	uint32_t zutexes[NUM_ZUTEXES];
} ZE;

void init_ze(ZE *ze)
{
	ze->dispatch_table = xil_get_dispatch_table();
	int i;
	for (i=0; i<NUM_ZUTEXES; i++)
	{
		ze->zutexes[i] = i;
	}
}

typedef struct {
	int i;
	ZE *ze;
	char rand_state_buf[16];
	struct random_data rand_buf;
	pthread_t pt;
	char debugstr[250];
} ZT;

int zt_rand(ZT *zt)
{
/*
	int rc;
	rc = random_r(&zt->rand_buf, &what);
	assert(rc==0);
*/
	return random();
}

int specsort(const void *vs0, const void *vs1)
{
	const XaxZutexWaitSpec *s0 = vs0;
	const XaxZutexWaitSpec *s1 = vs1;
	return s0->zutex - s1->zutex;
}

void *zt_worker(void *arg)
{
	ZT *zt = (ZT*) arg;

	fprintf(stderr, "ztw[%d] here!\n", zt->i);
	while (1)
	{
		int what = random();

		what = what & 1;
		if (zt->i==0)
		{
			what = 0;
		}

		int which = random();
		which = which % NUM_ZUTEXES;

		switch (what)
		{
		case 0: // wake
		{
			int waitms = (random() % 5);
			if (waitms>0)
			{
				usleep(waitms*10000);
			}

			sprintf(zt->debugstr, "%02d wake(%02d)", zt->i, which);
			fprintf(stderr, "%s\n", zt->debugstr);
			(zt->ze->dispatch_table->xax_zutex_wake)(
				&zt->ze->zutexes[which],
				zt->ze->zutexes[which],
				1,
				0,
				NULL,
				0);
			sprintf(zt->debugstr, "%02d __woke(%02d)", zt->i, which);
			fprintf(stderr, "%s\n", zt->debugstr);
		}
		break;
		case 1: // wait
		{
			int numspecs = (random() % 2) + 1;
			XaxZutexWaitSpec *specs = alloca(sizeof(XaxZutexWaitSpec)*numspecs);

			int si;
			for (si=0; si<numspecs; si++)
			{
retry:
				which = random();
				which = which % NUM_ZUTEXES;
				uint32_t *zutex = &zt->ze->zutexes[which];
				int j;
				for (j=0; j<si; j++)
				{
					if (specs[j].zutex == zutex)
					{
						// rats, a repeat
						goto retry;
					}
				}

				specs[si].zutex = zutex;
				specs[si].match_val = *zutex;
			}
			zoog_qsort(specs, numspecs, sizeof(XaxZutexWaitSpec), specsort);

			char list_text[200];
			strcpy(list_text, "[");
			for (si=0; si<numspecs; si++)
			{
				char buf[10];
				sprintf(buf, "%02d%s",
					specs[si].match_val,
					(si<numspecs-1 ? "," : ""));
				strcat(list_text, buf);
			}
			strcat(list_text, "]");

			sprintf(zt->debugstr, "%02d wait(%s)", zt->i, list_text);
			fprintf(stderr, "%s\n", zt->debugstr);
			(zt->ze->dispatch_table->xax_zutex_wait)(specs, numspecs);
			sprintf(zt->debugstr, "%02d __waited(%s)", zt->i, list_text);
			fprintf(stderr, "%s\n", zt->debugstr);
		}
		break;
		default:
			assert(0);
		break;
		}
	}
}

void init_zt(ZT *zt, int i, ZE *ze)
{
	zt->i = i;
	zt->ze = ze;
//	initstate_r(zt->i, zt->rand_state_buf, sizeof(zt->rand_state_buf), &zt->rand_buf);
//	srandom_r(zt->i, &zt->rand_buf);
	pthread_create(&zt->pt, NULL, zt_worker, zt);
}

int main(void)
{
	ZE ze;
	init_ze(&ze);

	//debug_reset_port();

	ZT zt[NUM_THREADS];
	memset(zt, 7, sizeof(zt));
	int i;
	for (i=0; i<NUM_THREADS; i++)
	{
		init_zt(&zt[i], i, &ze);
	}

	while (1)
	{
		sleep(1);
		for (i=0; i<NUM_THREADS; i++)
		{
			fprintf(stderr, "...%s\n", zt[i].debugstr);
		}
		fprintf(stderr, "\n");
	}

	for (i=0; i<NUM_THREADS; i++)
	{
		pthread_join(zt[i].pt, NULL);
	}
}
