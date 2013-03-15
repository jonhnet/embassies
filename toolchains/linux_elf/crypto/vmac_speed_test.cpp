#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <fstream>
#include "crypto.h"
#include "RandomSupply.h"
#include "MacKey.h"
#include "timer.h"
#include "GTimer.h"
#include "standard_malloc_factory.h"
#include "ambient_malloc.h"

using namespace std;

int main(int argc, char **argv)
{
	MallocFactory* mf = standard_malloc_factory_init();
	ambient_malloc_init(mf);

	initTimers();

	ifstream urand;
	urand.open("/dev/urandom", ios::binary);
	assert(!urand.fail());
	uint8_t buffer[RandomSupply::SEED_SIZE];
	urand.read((char*)buffer, sizeof(buffer));
	urand.close();
	RandomSupply* random_supply = new RandomSupply(buffer);

	int rc;
	uint32_t len;
	uint8_t* buf;
	assert(argc==2);

	if (atoi(argv[1])>1<<20)
	{
		len = atoi(argv[1]);
		buf = (uint8_t*) malloc(len);
	}
	else
	{
		FILE *fp = fopen(argv[1], "rb");
		rc = fseek(fp, 0, SEEK_END);
		assert(rc==0);
		len = ftell(fp);
		rc = fseek(fp, 0, SEEK_SET);
		assert(rc==0);
		buf = (uint8_t*) malloc(len);
		rc = fread(buf, len, 1, fp);
		assert(rc==1);
	}

	fprintf(stderr, "MACing %dMB file\n", len>>20);

	MacKey* macKey = new MacKey(random_supply);

#define NUM_SPEED_TRIALS 100
	GTimer gt;
	gt.start();
	Timer t;
	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		//MAC_compute(inputBuffer, ZFTP_BLOCK_SIZE, nonce, &ctx, mac);		
		Mac* mac = macKey->mac(random_supply, buf, len);
		delete mac;
	}
	stopTimer(&t);
	gt.stop();
	fprintf(stderr, "tot time %.3lfs\n", (double) (getTimeSeconds(&t)));
	fprintf(stderr, "avg time %.3lfs\n", (double) (getTimeSeconds(&t)/NUM_SPEED_TRIALS));
	fprintf(stderr, "syscall-based time: %.3lfs\n", gt.getTimeSeconds());
}
