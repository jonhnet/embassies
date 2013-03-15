#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <string.h>

#include "crypto.h"

#include "ZCert.h"
#include "ZKeyPair.h"
#include "RandomSupply.h"
#include "MacKey.h"
#include "ambient_malloc.h"
#include "standard_malloc_factory.h"
#include "pal_abi/pal_abi.h"
#include "AppIdentity.h"
#include "timer.h"

extern "C" {
#include "get_xax_dispatch_table.h"
}

bool test_endorsement(ZoogDispatchTable_v1* zdt) {
	uint8_t buffer[RandomSupply::SEED_SIZE];
	zdt->zoog_get_random(RandomSupply::SEED_SIZE, buffer);
	RandomSupply* random_supply = new RandomSupply(buffer);
	ZKeyPair* keys = new ZKeyPair(random_supply);
	uint8_t cert_buffer[ZCert::MAX_CERT_SIZE];

	zdt->zoog_endorse_me(keys->getPubKey(), ZCert::MAX_CERT_SIZE, cert_buffer);

	try {
		ZCert* cert = new ZCert(cert_buffer, ZCert::MAX_CERT_SIZE);
		if (!cert->getSpeakerKey()->isEqual(keys->getPubKey())) {
			fprintf(stderr, "Speaker key doesn't match what we generated!\n");
			return false;
		}
	} catch (...) { 
	  fprintf(stderr, "Caught an exception while trying to build a cert returned by PAL\n");
		return false;
	}


	return true;
}

bool test_app_secret(ZoogDispatchTable_v1* zdt) {
	uint8_t buffer[16];
	uint8_t buff[16];

	// Ask for the secret twice -- should be the same!
	bzero(buffer, sizeof(buffer));
	uint32_t ret1 = zdt->zoog_get_app_secret(sizeof(buffer), buffer);
	
	bzero(buff, sizeof(buff));
	uint32_t ret2 = zdt->zoog_get_app_secret(sizeof(buff), buff);

	if (ret1 != ret2) {
		fprintf(stderr, "Number of secret bytes returned is inconsistent: %d != %d\n",
		        ret1, ret2);
		return false;
	}

	for (uint32_t i = 0; i < sizeof(buff); i++) {
		if (buffer[i] != buff[i]) {
			fprintf(stderr, "Secret was inconsistent: buffer[%d] = %d, buff[%d] = %d\n",
			        i, buffer[i], i, buff[i]);
			return false;
		}
	}

	// Make sure we have something non-zero for a secret 
	// (hard to meaningfully test much else, unless we want to get into the Diehard realm)
	bool nonzero = false;
	for (uint32_t i = 0; i < sizeof(buff); i++) {
		if (buffer[i] != 0) {
			nonzero = true;
		}
	}
	if (!nonzero) {
		fprintf(stderr, "Secret was all 0s!\n"); 
		return false;
	}


	fprintf(stdout, "INFO: Asked for %d secret bytes, got %d back\n", sizeof(buffer), ret1);

	return true;
}

int main(int argc, char **argv, const char **envp)
{
	ambient_malloc_init(standard_malloc_factory_init());
	initTimers();
	
	fprintf(stdout, "About to hash...\n");
	uint8_t buffer[100];
	hash_t hash;
	for (uint32_t i=0; i<sizeof(buffer); i++) {
		buffer[i] = (uint8_t)i;
	}

	zhash(buffer, sizeof(buffer), &hash);
	fprintf(stdout, "Hashed!\n");

	uint8_t buff[RandomSupply::SEED_SIZE];
	for (uint32_t i=0; i<sizeof(buff); i++) {
		buff[i] = (uint8_t)i;
	}
	RandomSupply* random_supply = new RandomSupply(buff);
	MacKey* macKey = new MacKey(random_supply);
	Mac* mac = macKey->mac(random_supply, buffer, sizeof(buffer));
	delete mac;
	fprintf(stdout, "MAC'ed!\n");

#define FAKE_ZARFILE_SIZE 90*1024*1024
	uint8_t* fake_zarfile = new uint8_t[FAKE_ZARFILE_SIZE];
	random_supply->get_random_bytes(fake_zarfile, FAKE_ZARFILE_SIZE);

	Timer t;
	startTimer(&t);
	macKey->mac(random_supply, fake_zarfile, FAKE_ZARFILE_SIZE);
	stopTimer(&t);

	printf("MAC of zarfile-like object with %d bytes took %f seconds\n", FAKE_ZARFILE_SIZE, getTimeSeconds(&t));

	AppIdentity app_identity;

	ZCertChain chain;
	chain.addCert(app_identity.get_cert());
		// Only one link in the chain at the moment

	ZoogDispatchTable_v1* zdt = (ZoogDispatchTable_v1 *) get_xax_dispatch_table(envp);
	if (!zdt->zoog_verify_label(&chain)) {
		fprintf(stderr, "Label didn't verify!\n");
		return 3;
	}
	
	if (!test_endorsement(zdt)) {
		fprintf(stderr, "Endorsement failed!\n");
		return 4;
	}

	if (!test_app_secret(zdt)) {
		fprintf(stderr, "App secret fetch failed!\n");
		return 5;
	}

	fprintf(stdout, "Zoog identity test passed!\n");
	return 0;
}
