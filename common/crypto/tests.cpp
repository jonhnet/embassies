#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#ifndef _WIN32
#include <string.h>
#include <math.h>
#endif // !_WIN32

#include "timer.h"
#include "crypto.h"
#include "ZCert.h"
#include "ZCertChain.h"
#include "ZKeyPair.h"
#include "CryptoException.h"
#include "ZBinaryRecord.h"
#include "ZKeyLinkRecord.h"
#include "RandomSupply.h"
#ifdef NACL
#include "nacl/crypto_box.h"
#endif // NACL
#include "ambient_malloc.h"
#include "standard_malloc_factory.h"
#include "ZSymKey.h"
#include "PrfKey.h"
#include "MacKey.h"
#include "SymEncKey.h"
#include "KeyDerivationKey.h"
#include "LiteLib.h"

using namespace std;

#define NUM_SPEED_TRIALS 100 // 100

#define ZFTP_LG_BLOCK_SIZE     (15)
#define ZFTP_BLOCK_SIZE (1<<ZFTP_LG_BLOCK_SIZE)

void runHashSpeedTests() {
	uint8_t inputBuffer[ZFTP_BLOCK_SIZE];
	uint8_t hash[SPH_SIZE_sha512];
	Timer t;

	// Fill input with random data
	for (int i = 0; i < ZFTP_BLOCK_SIZE; i++) {
		inputBuffer[i] = rand() % 256;
	}

	/*****************************
	 *      SHA-256
	 *****************************/
	startTimer(&t);
	sph_sha256_context sha_ctx; 
	sph_sha256_init(&sha_ctx); 
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		sph_sha256(&sha_ctx, inputBuffer, ZFTP_BLOCK_SIZE); 
		sph_sha256_close(&sha_ctx, hash); 
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of SHA-256 on block size " << ZFTP_BLOCK_SIZE << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/SHA\n";
	cout << "Equivalent to: " << (getTimeCycles(&t) / (double)NUM_SPEED_TRIALS) / (double)ZFTP_BLOCK_SIZE << " cycles/byte\n";
	cout << endl;

	/*****************************
	 *      SHA-512
	 *****************************/
	startTimer(&t);
	sph_sha512_context sha512_ctx; 
	sph_sha512_init(&sha512_ctx); 
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		sph_sha512(&sha512_ctx, inputBuffer, ZFTP_BLOCK_SIZE); 
		sph_sha512_close(&sha512_ctx, hash); 
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of SHA-512 on block size " << ZFTP_BLOCK_SIZE << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/SHA\n";
	cout << "Equivalent to: " << (getTimeCycles(&t) / (double)NUM_SPEED_TRIALS) / (double)ZFTP_BLOCK_SIZE << " cycles/byte\n";
	cout << endl;
}


void runMacSpeedTests(RandomSupply* random) {
	uint8_t inputBuffer[ZFTP_BLOCK_SIZE];
	Timer t;

	// Fill input with random data
	for (int i = 0; i < ZFTP_BLOCK_SIZE; i++) {
		inputBuffer[i] = rand() % 256;
	}

	/*****************************
	 *      VMAC
	 *****************************/
	MacKey* macKey = new MacKey(random);

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		//MAC_compute(inputBuffer, ZFTP_BLOCK_SIZE, nonce, &ctx, mac);		
		Mac* mac = macKey->mac(random, inputBuffer, ZFTP_BLOCK_SIZE);
		delete mac;
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of VMAC on block size " << ZFTP_BLOCK_SIZE << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/VMAC\n";
	cout << "Equivalent to: " << (getTimeCycles(&t) / (double)NUM_SPEED_TRIALS) / (double)ZFTP_BLOCK_SIZE << " cycles/byte\n";
	cout << endl;

}

void runSymEncSpeedTests() {
	uint8_t inputBuffer[ZFTP_BLOCK_SIZE];

	// Fill input with random data
	for (int i = 0; i < ZFTP_BLOCK_SIZE; i++) {
		inputBuffer[i] = rand() % 256;
	}

	/*****************************
	 *      AES
	 *****************************/
#define AES_KEY_SIZE 128 /* bits */
/* TODO: Update with new symmetric key interface
	uint8_t aes_key[AES_KEY_SIZE / 8];
	uint8_t IV[MAX_IV_SIZE];
	uint8_t aes_output[ZFTP_BLOCK_SIZE + 2 * BITSPERBLOCK / 8]; 
	Timer t;

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		rijndael_encrypt(aes_key, AES_KEY_SIZE, inputBuffer, ZFTP_BLOCK_SIZE, IV, aes_output);
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of AES-128-CBC on block size " << ZFTP_BLOCK_SIZE << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/AES-128-CBC\n";
	cout << "Equivalent to: " << (getTimeCycles(&t) / (double)NUM_SPEED_TRIALS) / (double)ZFTP_BLOCK_SIZE << " cycles/byte\n";
	cout << "Equivalent to: " << (double)ZFTP_BLOCK_SIZE / (getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS) / pow(10.0,6)  << " Mbytes/second\n";
	cout << endl;
	*/
}

void runAsymKeyGenSpeedTests(RandomSupply* random_supply) {
	Timer t;

	/*****************************
	 *      RSA
	 *****************************/
	rsa_context rsa_ctx;
#define PUB_KEY_SIZE 2048

	rsa_init(&rsa_ctx, RSA_PKCS_V15, RSA_SHA256);

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		int result = rsa_gen_key(&rsa_ctx, PUB_KEY_SIZE, 65537, random_supply);
		if (result) {
			cout << "Error encrypting: " << result << endl;
		}
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of RSA-2048 key generation took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,3) << " milliseconds/keygen\n";
	cout << endl;

	rsa_free(&rsa_ctx);


	/*****************************
	 *      NaCl
	 *****************************/
#ifdef NACL
  unsigned char pk[crypto_box_PUBLICKEYBYTES];
  unsigned char sk[crypto_box_SECRETKEYBYTES];

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		crypto_box_keypair(pk, sk);
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of NaCl key generation took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,3) << " milliseconds/keygen\n";
	cout << endl;
#endif // NACL

}


void runAsymEncSpeedTests(RandomSupply* random_supply) {
	Timer t;

	/*****************************
	 *      RSA
	 *****************************/
	rsa_context rsa_ctx;
#define PUB_KEY_SIZE 2048
	uint8_t plain[PUB_KEY_SIZE / 8 - 11]; // 11 to allow for padding
	uint8_t cipher[PUB_KEY_SIZE / 8];

	// Fill input with random data
	for (uint32_t i = 0; i < sizeof(plain); i++) {
		plain[i] = i % 256;
	}

	rsa_init(&rsa_ctx, RSA_PKCS_V15, RSA_SHA1);
	int result = rsa_gen_key(&rsa_ctx, PUB_KEY_SIZE, 65537, random_supply);
	if (result) {
		cout << "Error when generating RSA key: " << result << endl;
	}

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		result = rsa_pkcs1_encrypt(&rsa_ctx, random_supply, RSA_PUBLIC, sizeof(plain), plain, cipher);
		if (result) {
			cout << "Error encrypting: " << result << endl;
		}
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of RSA-2048 encrypting on message size " << sizeof(plain) << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/enc\n";
	cout << endl;

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		int plainLen = 0;
		result = rsa_pkcs1_decrypt(&rsa_ctx, RSA_PRIVATE, &plainLen, cipher, plain, sizeof(plain));
		if (result) {
			cout << "Error decrypting: " << result << endl;
		}
		//cout << "Decrypted length = " << plainLen << endl;
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of RSA-2048 decrypting on cipher size " << sizeof(cipher) << " took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/enc\n";
	cout << endl;

	
	for (uint32_t i = 0; i < sizeof(plain); i++) {
		//cout << "plain[" << i << "] = " << (int) plain[i] << endl;	
		if (plain[i] != i % 256) {
			cout << "Error!  Decrypted byte " << i << " doesn't match what we asymetrically encrypted!\n";
			break;
		}
	}

	rsa_free(&rsa_ctx);
}

void runSigSpeedTests(RandomSupply* random_supply) {
	Timer t;

	/*****************************
	 *      RSA
	 *****************************/
	rsa_context rsa_ctx;
#define PUB_KEY_SIZE 2048
	uint8_t plain[PUB_KEY_SIZE / 8 - 11]; // 11 to allow for padding
	uint8_t cipher[PUB_KEY_SIZE / 8];
	uint8_t hash[SPH_SIZE_sha512];

	// Fill input with "random" data
	for (uint32_t i = 0; i < sizeof(plain); i++) {
		plain[i] = i % 256;
	}

	// Fill hash with "random" data
	for (uint32_t i = 0; i < sizeof(hash); i++) {
		hash[i] = i % 256;
	}

	rsa_init(&rsa_ctx, RSA_PKCS_V15, RSA_SHA1);
	int result = rsa_gen_key(&rsa_ctx, PUB_KEY_SIZE, 65537, random_supply);
	if (result) {
		cout << "Error when generating RSA key: " << result << endl;
	}

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		result = rsa_pkcs1_sign(&rsa_ctx, RSA_PRIVATE, RSA_SHA1, SPH_SIZE_sha512, hash, cipher);
		if (result) {
			cout << "Error signing: " << result << endl;
		}
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of RSA-2048 signing on hash size 128 took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/sig\n";
	cout << endl;

	startTimer(&t);
	for (int trial = 0; trial < NUM_SPEED_TRIALS; trial++) {
		result = rsa_pkcs1_verify(&rsa_ctx, RSA_PUBLIC, RSA_SHA1, SPH_SIZE_sha512, hash, cipher);
		if (result) {
			cout << "Error verifying: " << result << endl;
		} 
		//else {
		//	cout << "Verification succeeded!" << endl;
		//}
	}
	stopTimer(&t);

	cout << NUM_SPEED_TRIALS << " of RSA-2048 verifying on hash size 128 took total time: " << getTimeSeconds(&t) << " seconds\n";
	cout << "Average time: " << getTimeSeconds(&t) / (double) NUM_SPEED_TRIALS * pow(10.0,6) << " microseconds/verify\n";
	cout << endl;

	rsa_free(&rsa_ctx);
}


	/*result = rsa_check_pubkey(&rsa_ctx);
	if (result) {
		cout << "Error checking public RSA key: " << result << endl;
	}

	result = rsa_check_privkey(&rsa_ctx);
	if (result) {
		cout << "Error checking priv RSA key: " << result << endl;
	}*/

bool testGoodDomainNameCreation(char* name, uint32_t size, uint8_t length, uint8_t numLabels) {
	bool retval = false;
	DomainName *d = NULL;

	try {
		d = new DomainName(name, size);	
		assert(d);

		if (!d || 
			d->size() != length || 
			d->numLabels() != numLabels ||
			strcmp(d->toString(), name) != 0) {
			cout << "Failed DomainName creation test with name: " << name << endl;
			retval = false;
		} else {
			retval = true;
		}
	} catch (...) {
		cout << "Exceptional failure of GoodDomainName creation test with name: " << name << endl;
		retval = false;
	}

	delete d;
	return retval;
}


bool testBadDomainNameCreation(char* name, uint8_t length) {
	DomainName *d = NULL;
	bool retval = false;

	try {
		d = new DomainName(name, length);			
		retval = false;		// Should have thrown an exception
	}  catch (CryptoException e) {
		if (e.getType() == CryptoException::BAD_INPUT) {
			// We pass!
			retval = true;
		} else {
			cout << "Got an unexpected exception: "	<< e << " during BadDomainName creation test with name: " << name << endl;			
			retval = false;
		}	
	} catch (...) {
		cout << "Exceptional failure of BadDomainName creation test with name: " << name << endl;
		retval = false;
	}

	delete d;
	return retval;
}

bool testDomainNameSerialization(DomainName* d) {
	bool retval = false;

	uint8_t* buffer = new uint8_t[d->size()];
	assert(buffer);

	DomainName* d2 = NULL;
	try {
		d->serialize(buffer);
		d2 = new DomainName(buffer, d->size());	// Resurrect it
		assert(d2);

		if (!d2 ||  
			*d != *d2) {
			retval = false;
			cout << "Failed to properly serialize and deserialize DomainName: " << d->toString() << endl;
		} else {
			retval = true;
		}
	} catch (...) {
		cout << "Exceptional failure of DomainNameSerialization test with DomainName: " << d->toString() << endl;
		retval = false;
	}

	delete [] buffer;
	delete d2;

	return retval;
}

bool testBadDomainNameSerialization(uint8_t* buffer, uint8_t length, int id) {
	DomainName *d = NULL;
	bool retval = false;

	try {
		d = new DomainName(buffer, length);			
		retval = false;		// Should have thrown an exception
	}  catch (CryptoException e) {
		if (e.getType() == CryptoException::BAD_INPUT) {
			// We pass!
			retval = true;
		} else {
			cout << "Got an unexpected exception: "	<< e << " during BadDomainName serialization test #" << id << endl;	
			retval = false;
		}	
	} catch (...) {
		cout << "Exceptional failure of BadDomainName serialization test #" << id << endl;	
		retval = false;
	}

	delete d;
	return retval;
}


bool testRecordSerialization(ZRecord* r, int id) {
	bool retval = false;

	uint8_t* buffer = new uint8_t[r->size()];
	assert(buffer);

	ZRecord* r2 = NULL;
	try {
		r->serialize(buffer);
		r2 = ZRecord::spawn(buffer, r->size());	// Resurrect it

		if (!r2 ||  
			!r->isEqual(r2)) {
			retval = false;
			cout << "Failed to properly serialize and deserialize record, test #" << id << endl;
		} else {
			retval = true;
		}
	} catch (...) {
		cout << "Exceptional failure of record serialization test #" << id << endl;
		retval = false;
	}

	delete [] buffer;
	delete r2;

	return retval;
}

bool testBadRecordSerialization(uint8_t* buffer, uint8_t length, int id) {
	ZRecord *r = NULL;
	bool retval = false;

	try {
		r = ZRecord::spawn(buffer, length);			
		retval = false;		// Should have thrown an exception
	}  catch (CryptoException e) {
		if (e.getType() == CryptoException::BAD_INPUT) {
			// We pass!
			retval = true;
		} else {
			cout << "Got an unexpected exception: "	<< e << " during Bad Record serialization test #" << id << endl;	
			retval = false;
		}	
	} catch (...) {
		cout << "Exceptional failure of Bad Record serialization test #" << id << endl;	
		retval = false;
	}

	delete r;
	return retval;
}


bool testCertSerialization(ZCert* c, int id) {
	bool retval = false;

	uint8_t* buffer = new uint8_t[c->size()];
	assert(buffer);

	ZCert* c2 = NULL;
	try {
		c->serialize(buffer);
		c2 = new ZCert(buffer, c->size());	// Resurrect it
		assert(c2);

		if (!c2 ||  
			*c != *c2) {
			retval = false;
			cout << "Failed to properly serialize and deserialize cert id: " << id << endl;
		} else {
			retval = true;
		}
	} catch (...) {
		cout << "Exceptional failure of ZCert serialization test with cert id: " << id << endl;
		retval = false;
	}

	delete [] buffer;
	delete c2;

	return retval;
}

bool testBadCertSerialization(uint8_t* buffer, uint8_t length, int id) {
	ZCert *r = NULL;
	bool retval = false;

	try {
		r = new ZCert(buffer, length);			
		retval = false;		// Should have thrown an exception
	}  catch (CryptoException e) {
		if (e.getType() == CryptoException::BAD_INPUT) {
			// We pass!
			retval = true;
		} else {
			cout << "Got an unexpected exception: "	<< e << " during Bad Cert serialization test #" << id << endl;	
			retval = false;
		}	
	} catch (...) {
		cout << "Exceptional failure of Bad Cert serialization test #" << id << endl;	
		retval = false;
	}

	delete r;
	return retval;
}

int runDomainNameTests() {
	int numTestsFailed = 0;

	/* len = 1 byte for the root label */
	if (!testGoodDomainNameCreation((char*)" ", sizeof(" "), 1, 0)) { numTestsFailed++; } 

	/* len = 1 for the root, 1 for label len, 3 for label */
	if (!testGoodDomainNameCreation((char*)"com", sizeof("com"), 5, 1)) { numTestsFailed++; }  

	/* len = 1 for the root, 2 for label lens, 7+3 for labels */
	if (!testGoodDomainNameCreation((char*)"example.com", sizeof("example.com"), 13, 2)) { numTestsFailed++; }  

	/* len = 1 for the root, 3 for label lens, 3+7+3 for labels */
	if (!testGoodDomainNameCreation((char*)"www.example.com", sizeof("www.example.com"), 17, 3)) { numTestsFailed++; }  

	DomainName *d1 = new DomainName(" ", sizeof(" "));	
	DomainName *d2 = new DomainName("com", sizeof("com"));	
	DomainName *d3 = new DomainName("example.com", sizeof("example.com"));	
	DomainName *d4 = new DomainName("www.example.com", sizeof("www.example.com"));	

	// Test suffix
	if (! (d4->suffix(d3) &&
		   d3->suffix(d2) &&
		   d2->suffix(d1)) ) {
		numTestsFailed++;
		cout << "Failed correct suffix test!\n";
	}

	if (d1->suffix(d2) ||
		d1->suffix(d3) ||
		d1->suffix(d4) ||
		d2->suffix(d3) ||
		d2->suffix(d4) ||
		d3->suffix(d4) ) {
		numTestsFailed++;
		cout << "Failed incorrect suffix test!\n";
	}

	DomainName* d5 = new DomainName("www.examplecom", sizeof("www.examplecom"));
	DomainName* d6 = new DomainName("wwwexample.com", sizeof("wwwexample.com"));
	DomainName* d7 = new DomainName("www.e1xample.com", sizeof("www.e1xample.com"));
	DomainName* d8 = new DomainName("www.example1.com", sizeof("www.example1.com"));

	if ( d5->suffix(d2) || d5->suffix(d3) || d5->suffix(d4) ||
		 d6->suffix(d3) || d6->suffix(d4) ||
		 d7->suffix(d3) || d7->suffix(d4) ||
		 d8->suffix(d3) || d8->suffix(d4)) {
		numTestsFailed++;
		cout << "Failed suffix test 2!\n";
	}

	// Try various "bad" domain names
	if(!testBadDomainNameCreation(NULL, 10)) { numTestsFailed++; }  
	/* 
	These currently fail!
	if(!testBadDomainNameCreation(".")) { numTestsFailed++; }  
	if(!testBadDomainNameCreation("..")) { numTestsFailed++; }  
	if(!testBadDomainNameCreation("com.")) { numTestsFailed++; }  
	if(!testBadDomainNameCreation(".com.")) { numTestsFailed++; }  
	if(!testBadDomainNameCreation("example..com")) { numTestsFailed++; }
	*/

	// Test equality
	if (   *d1 != *d1 
		|| ! (*d1 == *d1) 
		|| ! (*d2 == *d2) 
		|| ! (*d3 == *d3) 
		|| ! (*d4 == *d4) 
		|| ! (*d5 == *d5) 
		|| ! (*d6 == *d6) 
		|| ! (*d7 == *d7) 
		|| ! (*d8 == *d8) 
		|| *d1 == *d2
		|| *d2 == *d3
		|| *d4 == *d5
		|| *d6 == *d7
		|| *d8 == *d1 ){
		cout << "DomainName equality test failed!\n";
		numTestsFailed++;
	}

	// Test serialization
	if(!testDomainNameSerialization(d1)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d2)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d3)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d4)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d5)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d6)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d7)) { numTestsFailed++; }  
	if(!testDomainNameSerialization(d8)) { numTestsFailed++; }  

	// Test badly formatted "serialized" DomainNames
	if(!testBadDomainNameSerialization(NULL, 100, 1)) { numTestsFailed++; }  

	uint8_t b2[] = {0x1, 0x2, 0x3};  // Label length (0x3) at the end is a lie
	if(!testBadDomainNameSerialization(b2, sizeof(b2), 2)) { numTestsFailed++; }  
	
	uint8_t b3[] = {0x1, 0x65};	// Missing root label (0x0) at the end
	if(!testBadDomainNameSerialization(b3, sizeof(b3), 3)) { numTestsFailed++; }  

	// Do some basic fuzzing
	uint8_t buffer[100];
	for (int i = 0; i < 1000; i++) {
		// Fill buffer with random data
		for (int j = 0; j < 100; j++) {
			buffer[j] = rand() % 256; 
		}
		// Make sure the first item is not 0 (which is a valid encoding)
		buffer[0] = rand() % 255 + 1;

		// Make sure subsequent lengths aren't zero (or it's valid!)
		uint32_t pos = buffer[0] + 1;
		while (pos < sizeof(buffer)) {
			if (buffer[pos] == 0) {
				buffer[pos] = rand() % 255 + 1;
			}
			pos += buffer[pos] + 1;
		}
		
		if(!testBadDomainNameSerialization(buffer, sizeof(buffer), i + 4)) { numTestsFailed++; }  
	}

	delete d1;
	delete d2;
	delete d3;
	delete d4;
	delete d5;
	delete d6;
	delete d7;
	delete d8;

	return numTestsFailed;
}

int runRecordTests(RandomSupply* random_supply) {
	uint32_t numTestsFailed = 0;

	/***********************
	 *    ZBinaryRecord    *
	 ***********************/
#define BINARY_SIZE 100
	uint8_t binary[BINARY_SIZE];

	// Fill binary
	for (uint32_t i = 0; i < sizeof(binary); i++) {
		binary[i] = i % 256;
	}

	DomainName* name = new DomainName("www.example.com", sizeof("www.example.com"));

	ZBinaryRecord* b1 = new ZBinaryRecord(binary, BINARY_SIZE);
	if(!testRecordSerialization(b1, 1)) { numTestsFailed++; }  

	ZBinaryRecord* b2 = new ZBinaryRecord(binary, BINARY_SIZE / 2);
	if(!testRecordSerialization(b2, 2)) { numTestsFailed++; } 

	ZBinaryRecord* b3 = new ZBinaryRecord(binary, BINARY_SIZE);
	if(!testRecordSerialization(b3, 3)) { numTestsFailed++; } 

	if (!(b1->isEqual(b1) && b2->isEqual(b2) && b3->isEqual(b3) && b1->isEqual(b3))) {
		cout << "Failed binary record equality tests!\n";
		numTestsFailed++;
  }

  if (b1->isEqual(b2) || b3->isEqual(b2) || b2->isEqual(b1)) {
		cout << "Failed binary record inequality tests!\n";
		numTestsFailed++;
	}

	// Try some bad serializations by corrupting a correct one
	uint8_t* buffer = new uint8_t[b1->size()];
	b1->serialize(buffer);
	for (uint32_t i = 1; i < b1->size(); i++) {
		if(!testBadRecordSerialization(buffer, b1->size() - i, i)) { numTestsFailed++; } 
	}

	delete b1;
	delete b2;
	delete b3;
	delete [] buffer;
	
	/***********************
	 *    ZPubKeyRecord    *
	 ***********************/
#define KEY_SIZE 100
	buffer = new uint8_t[KEY_SIZE];
	assert(buffer); 

	// Fill key
	for (uint32_t i = 0; i < sizeof(buffer); i++) {
		buffer[i] = i % 256;
	}

	ZPubKeyRecord* z1 = new ZPubKeyRecord(name, buffer, sizeof(buffer));
	if(!testRecordSerialization(z1, 4)) { numTestsFailed++; }  

	ZPubKeyRecord* z2 = new ZPubKeyRecord(name, buffer, sizeof(buffer) / 2);
	if(!testRecordSerialization(z1, 5)) { numTestsFailed++; }  

	ZPubKeyRecord* z3 = new ZPubKeyRecord(name, buffer, sizeof(buffer));
	if(!testRecordSerialization(z1, 6)) { numTestsFailed++; }  

	if (! (z1->isEqual(z1) && z2->isEqual(z2) && z3->isEqual(z3) && z1->isEqual(z3))
		|| z1->isEqual(z2) || z3->isEqual(z2) || z2->isEqual(z1)) {
		cout << "Failed pub key record equality tests!\n";
		numTestsFailed++;
	}

	if (z1->getTag() != z1->getTag() || z2->getTag() != z2->getTag() ||
		z1->getTag() == z2->getTag()) {
		cout << "Failed pub key record tag equality tests!\n";
		numTestsFailed++;
	}

	ZKeyPair* zk = new ZKeyPair(random_supply, name);
	ZPubKeyRecord* z4 = zk->getPubKey()->getRecord();
	ZPubKey* pk = z4->getKey();
	if (pk->getTag() != zk->getPubKey()->getTag()) {
		cout << "Failed pub key record key tag equality test!\n";
		numTestsFailed++;
	}

	delete zk;

	// Try some bad serializations by corrupting a correct one
	buffer = new uint8_t[z1->size()];
	z1->serialize(buffer);
	for (uint32_t i = 1; i < z1->size(); i++) {
		if(!testBadRecordSerialization(buffer, z1->size() - i, i)) { numTestsFailed++; } 
	}

	delete z1;
	delete z2;
	delete z3;
	delete [] buffer;

	/***********************
	 *    ZSigRecord       *
	 ***********************/
	ZSigRecord* s1 = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_BINARY, name, 50, 100);
	ZSigRecord* s2 = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_BINARY, name, 5, 10);
	ZSigRecord* s3 = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_DELEGATION, name, 5, 10);
	ZSigRecord* s4 = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_KEY, name, 5, 10);
	ZSigRecord* s5 = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_BINARY, name, 50, 100);

	if(!testRecordSerialization(s1, 7)) { numTestsFailed++; }  
	if(!testRecordSerialization(s2, 8)) { numTestsFailed++; }  
	if(!testRecordSerialization(s3, 9)) { numTestsFailed++; }  
	if(!testRecordSerialization(s4, 10)) { numTestsFailed++; }  

	if (! (s1->isEqual(s1) && s2->isEqual(s2) && s3->isEqual(s3) && 
		     s4->isEqual(s4) && s5->isEqual(s5) && s1->isEqual(s5))) {
		cout << "Failed sig record equality tests!\n";
		numTestsFailed++;
	}	

	if (s1->isEqual(s2) || s2->isEqual(s3) || s3->isEqual(s4) ||
		  s4->isEqual(s5) || s2->isEqual(s4) || s2->isEqual(s5)) {
		cout << "Failed sig record inequality tests!\n";
		numTestsFailed++;
	}

	DomainName* name2 = new DomainName("foo.bar.com", sizeof("foo.bar.com"));
	s5->setSignerName(name2);
	if (s1->isEqual(s5)) {
		cout << "Failed sig record signerName test 1!\n";
		numTestsFailed++;
	}
	s1->setSignerName(name2);
	if (!s1->isEqual(s5)) {
		cout << "Failed sig record signerName test 2!\n";
		numTestsFailed++;
	}

	// Try some bad serializations by corrupting a correct one
	buffer = new uint8_t[s1->size()];
	s1->serialize(buffer);
	for (uint32_t i = 1; i < s1->size(); i++) {
		if(!testBadRecordSerialization(buffer, s1->size() - i, i + 1000)) { numTestsFailed++; } 
	}

	delete name2;
	delete s1;
	delete s2;
	delete s3;
	delete s4;
	delete s5;
	delete [] buffer;

	/***********************
	 *    ZKeyLinkRecord   *
	 ***********************/	
	ZKeyPair* keys1 = new ZKeyPair(random_supply);
	ZKeyPair* keys2 = new ZKeyPair(random_supply);

	ZKeyLinkRecord* k1 = new ZKeyLinkRecord(keys1->getPubKey(), keys2->getPubKey());
	ZKeyLinkRecord* k2 = new ZKeyLinkRecord(keys2->getPubKey(), keys1->getPubKey());
	ZKeyLinkRecord* k3 = new ZKeyLinkRecord(keys1->getPubKey(), keys2->getPubKey());
	
	if(!testRecordSerialization(k1, 11)) { numTestsFailed++; }  
	if(!testRecordSerialization(k2, 12)) { numTestsFailed++; }  
	if(!testRecordSerialization(k3, 13)) { numTestsFailed++; }  	

	if ( (!(k1->isEqual(k1) && k2->isEqual(k2) && k3->isEqual(k3)))
	    || !k1->isEqual(k3) || !k3->isEqual(k1)) {
		cout << "Failed key link record equality tests!\n";
		numTestsFailed++;
	}	

	if (k1->isEqual(k2) || k2->isEqual(k3)) {
		cout << "Failed key link record inequality tests!\n";
		numTestsFailed++;
	}

	// Try some bad serializations by corrupting a correct one
	buffer = new uint8_t[k1->size()];
	k1->serialize(buffer);
	for (uint32_t i = 1; i < k1->size(); i++) {
		if(!testBadRecordSerialization(buffer, k1->size() - i, i + 1000)) { numTestsFailed++; } 
	}

	delete k1;
	delete k2;
	delete k3;
	delete keys1;
	delete keys2;
	delete buffer;

	delete name;
	return numTestsFailed;
}

int runCertTests(RandomSupply* random_supply) {
	uint32_t numTestsFailed = 0;

#define BINARY_SIZE 100
	uint8_t binary[BINARY_SIZE];

	// Fill binary
	for (uint32_t i = 0; i < sizeof(binary); i++) {
		binary[i] = i % 256;
	}

	DomainName* name = new DomainName("www.example.com", sizeof("www.example.com"));
	ZKeyPair* zkp = new ZKeyPair(random_supply, name);
	ZKeyPair* zkp2 = new ZKeyPair(random_supply, name);


	ZCert* c1 = new ZCert(getCurrentTime() - 10000, getCurrentTime() + 10000, binary, sizeof(binary));
	ZCert* c2 = new ZCert(getCurrentTime() - 10000, getCurrentTime() + 10000, binary, sizeof(binary));
	ZCert* c3 = new ZCert(getCurrentTime() - 10000, getCurrentTime() + 10000, name, zkp->getPubKey());
	ZCert* c4 = new ZCert(getCurrentTime() - 10000, getCurrentTime() + 10000, zkp->getPubKey(), zkp2->getPubKey());

	if(!testCertSerialization(c1, 1)) { numTestsFailed++; }  	
	if(!testCertSerialization(c2, 2)) { numTestsFailed++; } 	
	if(!testCertSerialization(c3, 3)) { numTestsFailed++; }  
	if(!testCertSerialization(c4, 4)) { numTestsFailed++; }  

	if (*c1 != *c1 || *c2 != *c2 || *c3 != *c3 || *c4 != *c4 ||
		*c1 != *c2 || *c2 == *c3 || *c1 == *c3 || *c1 == *c4 || *c2 == *c4 || *c3 == *c4) {
		cout << "Failed cert equality tests!\n";
		numTestsFailed++;
	}

	// Try some bad serializations by corrupting a correct one
	uint8_t* buffer = new uint8_t[c1->size()];
	c1->serialize(buffer);
	for (uint32_t i = 1; i < c1->size(); i++) {
		if(!testBadCertSerialization(buffer, c1->size() - i, i)) { numTestsFailed++; } 
	}
	delete [] buffer;

	// Try signing and verifying the certs
	c1->sign(zkp->getPrivKey());
	if (!c1->verify(zkp->getPubKey())) {
		cout << "Failed basic cert sign test 1!\n";
		numTestsFailed++;
	}

	DomainName* name2 = new DomainName("foo.example.edu", sizeof("foo.example.edu"));
	ZKeyPair* zkp3 = new ZKeyPair(random_supply, name2);

	c2->sign(zkp3->getPrivKey());
	if (!c2->verify(zkp3->getPubKey())) {
		cout << "Failed basic cert sign test 2!\n";
		numTestsFailed++;
	}

	// Verification with the wrong key should fail
	if (c2->verify(zkp->getPubKey()) || c1->verify(zkp3->getPubKey())) {
		cout << "Failed basic cert sign test with bad key!\n";
		numTestsFailed++;
	}

	// Check that signatures still verify after serialization
	buffer = new uint8_t[c1->size()];
	c1->serialize(buffer);
	ZCert* c5 = new ZCert(buffer, c1->size());
	delete [] buffer;

	// Verification should still work
	if (!c5->verify(zkp->getPubKey()) || c5->verify(zkp3->getPubKey())) {
		cout << "Failed cert serialization sign test!\n";
		numTestsFailed++;
	} 

	// Check that we can ask for a cert's signing key and get it's size
	if (c1->getEndorsingKey()->size() != c1->getEndorsingKey()->size()) {
		cout << "Failed key size cert test!\n";
		numTestsFailed++;
	}

	// Size of the keys should be the same
	if (c1->getEndorsingKey()->size() != c2->getEndorsingKey()->size()) {
		cout << "Failed key size cert comparison test!\n";
		numTestsFailed++;
	}

	// Cleanup
	delete name;
	delete name2;
	delete zkp;
	delete zkp2;
	delete zkp3;
	delete c1;
	delete c2;
	delete c3;
	delete c4;
	delete c5;

	return numTestsFailed;
}

int runKeyTests(RandomSupply* random_supply) {
	uint32_t numTestsFailed = 0;

#define BINARY_SIZE 100
	uint8_t binary[BINARY_SIZE];

	// Fill binary
	for (uint32_t i = 0; i < sizeof(binary); i++) {
		binary[i] = i % 256;
	}

	DomainName* name = new DomainName("www.example.com", sizeof("www.example.com"));
	ZCert* c1 = new ZCert(getCurrentTime() - 10000, getCurrentTime() + 10000, binary, sizeof(binary));
	ZKeyPair* zkp = new ZKeyPair(random_supply, name);
	
	c1->sign(zkp->getPrivKey());

	// Try serializing and deserializing the keys
	uint32_t size = zkp->size();
	uint8_t* buffer = new uint8_t[size];
	zkp->serialize(buffer);
	delete zkp;

	zkp = new ZKeyPair(buffer, size);
	delete [] buffer;

	// Verification should still work
	if (!c1->verify(zkp->getPubKey())) {
		cout << "Failed key deserialization & verify test\n";
		numTestsFailed++;
	}

	// Test label verification
	DomainName* name0 = new DomainName(" ", sizeof(" "));
	ZKeyPair* keys0 = new ZKeyPair(random_supply, name0);

	DomainName* name1 = new DomainName("com", sizeof("com"));
	ZKeyPair* keys1 = new ZKeyPair(random_supply, name1);

	DomainName* name2 = new DomainName("example.com", sizeof("example.com"));
	ZKeyPair* keys2 = new ZKeyPair(random_supply, name2);

	DomainName* name3 = new DomainName("www.example.com", sizeof("www.example.com"));
	ZKeyPair* keys3 = new ZKeyPair(random_supply, name3);
	
	// Certify .com's key
	ZCert* cert0 = new ZCert(getCurrentTime() - 1000, getCurrentTime() + 1000, name1, keys1->getPubKey());
	cert0->sign(keys0->getPrivKey());

	// Certify example.com's key
	ZCert* certa = new ZCert(getCurrentTime() - 1000, getCurrentTime() + 1000, name2, keys2->getPubKey());
	certa->sign(keys1->getPrivKey());

	// Certify www.example.com's key
	ZCert* certb = new ZCert(getCurrentTime() - 1000, getCurrentTime() + 1000, name3, keys3->getPubKey());
	certb->sign(keys2->getPrivKey());
	
	ZCertChain chain;
	chain.addCert(cert0);
	chain.addCert(certa);
	chain.addCert(certb);
	
	if (!keys3->getPubKey()->verifyLabel(&chain, keys0->getPubKey())) {
		cout << "Failed label verification test!\n";
		numTestsFailed++;
	}

	delete name0;
	delete name1;
	delete name2;
	delete name3;	
	delete keys0;
	delete keys1;
	delete keys2;
	delete keys3;
	delete cert0;
	delete certa;
	delete certb;	

	return numTestsFailed;
}

int runRandTests() {
	uint32_t numTestsFailed = 0;

	uint8_t seed[RandomSupply::SEED_SIZE];
	for (uint32_t i = 0; i < sizeof(seed); i++) {
		seed[i] = rand() % 256;
	}

	RandomSupply* rand = new RandomSupply(seed);

	// Try some basic sanity tests
#define NUM_BINS 256
	uint32_t hist[NUM_BINS];
	lite_bzero(hist, sizeof(hist));

// Chernoff bound: If #balls = MULT*6*#bins*log(#bins)
// then max load is (1+1/sqrt(MULT))(#balls/#bins) with prob 1-1/#bins^2
#define MULT 100
#define NUM_RAND_TRIALS (6*NUM_BINS*log((float)NUM_BINS)*MULT)
	for (uint32_t i = 0; i < NUM_RAND_TRIALS; i++) {
		hist[rand->get_random_byte()]++;
	}

	uint32_t max = 0;
	uint32_t min = NUM_RAND_TRIALS + 1;
	for (uint32_t i = 0; i < 256; i++) {
		if (hist[i] < min) { min = hist[i]; }
		if (hist[i] > max) { max = hist[i]; }
	}
	cout << "Random counts: Min = "<<min<<", Max = " <<max<<endl;
	double predicted_avg = NUM_RAND_TRIALS / (double) NUM_BINS;
	double delta = 1.0/sqrt((float)MULT) * predicted_avg;
	if (min < predicted_avg - delta ||
			max > predicted_avg + delta) {
		cout << "Warning: These our outside expected limits!\n"
			   << "\t\t(Probability of this happening by chance is: "
		     << 1.0/pow((float)NUM_BINS,2) * 100 << "%%)\n";
		//cout << "Predicted avg: " << predicted_avg << " +/- " << delta <<endl;
		numTestsFailed++;
	}

	// Make sure buffer filling works too, with various lengths
	for (uint32_t i = 0; i < sizeof(hist); i++) {
		rand->get_random_bytes((uint8_t*)hist, i);
	}

	delete rand;
	return numTestsFailed;
}

int runSymKeyTests (RandomSupply* rand) {
	uint32_t numTestsFailed = 0;
	
	// Try creating the various types of keys
	KeyDerivationKey* kdf = new KeyDerivationKey(rand);

	KeyDerivationKey* kk = kdf->deriveKeyDerivationKey((uint8_t*)"KDF", 3);
	PrfKey* pk = kdf->derivePrfKey((uint8_t*)"PRF", 3);
	MacKey* mk = kdf->deriveMacKey((uint8_t*)"MAC", 3);
	SymEncKey* sk = kdf->deriveEncKey((uint8_t*)"Enc", 3);

	// Make sure all of the new keys work
	KeyDerivationKey* kkprime = kk->deriveKeyDerivationKey((uint8_t*)"KDFPrime", 3);
	delete kkprime;

	uint8_t input[PrfKey::BLOCK_SIZE];
	uint8_t output[PrfKey::BLOCK_SIZE];
	pk->evaluatePrf(input, output);

	uint8_t buffer[234];
	for (uint32_t i = 0; i < sizeof(buffer); i++) {
		buffer[i] = i % 256;
	}

	Mac* mac = mk->mac(rand, buffer, sizeof(buffer));
	if (!mk->verify(mac, buffer, sizeof(buffer))) {
		cout << "Failed mac verify test\n";
		numTestsFailed++;
	}

	// Even removing one byte should cause a failure
	if (mk->verify(mac, buffer, sizeof(buffer)-1)) {
		cout << "Failed mac length verify test - should have rejected\n";
		numTestsFailed++;
	}
	
	// Even changing one byte should cause a failure
	uint8_t tmp = buffer[7];
	buffer[7] = 41;
	if (mk->verify(mac, buffer, sizeof(buffer))) {
		cout << "Failed mac byte verify test - should have rejected\n";
		numTestsFailed++;
	}
	buffer[7] = tmp;
	delete mac;

	// Try MAC on a really small buffer
	uint8_t buf2[12];
	for (uint32_t i = 0; i < sizeof(buf2); i++) {
		buf2[i] = i % 256;
	}

	mac = mk->mac(rand, buf2, sizeof(buf2));
	if (!mk->verify(mac, buf2, sizeof(buf2))) {
		cout << "Failed mac verify small test\n";
		numTestsFailed++;
	}
	delete mac;

	Ciphertext* cipher = sk->encrypt(rand, buffer, sizeof(buffer));
	Plaintext* plain = sk->decrypt(cipher);
	
	if (plain == NULL) {
		cout << "Failed sym enc/dec test: bad integrity check\n";
		numTestsFailed++;
	} else {
		if (plain->len != sizeof(buffer)) {
			cout << "Failed sym enc/dec test: length mismatch\n";
			numTestsFailed++;
		}

		if (memcmp(plain->bytes, buffer, sizeof(buffer)) != 0) {
			cout << "Failed sym enc/dec test: result mismatch\n";
			for (uint32_t i = 0; i < sizeof(buffer); i++) {
				if (plain->bytes[i] != buffer[i]) {
					printf("Mismatch at i=%d: %x, %x\n", i, plain->bytes[i], buffer[i]); 
				}
			}
			numTestsFailed++;
		}

		delete plain;
	}

	// Decryption should fail if we change the ciphertext
	cipher->bytes[42] = ~cipher->bytes[42]; 
	plain = sk->decrypt(cipher);
	if (plain != NULL) {
		cout << "Failed sym enc/dec test: Didn't detect ciphertext change\n";
		numTestsFailed++;
	} else {
		delete plain;
	}

	delete cipher;

	delete kk;
	delete sk;
	delete mk;
	delete pk;
	delete kdf;
	return numTestsFailed;
}

int runTests(RandomSupply* random_supply, bool speed) {
	int result = 0;

	uint32_t seed = (unsigned int)time(NULL);
	cout << "Running tests with random seed: " << seed << endl;
	srand(seed);

	initTimers();	

	MallocFactory* mf = standard_malloc_factory_init();
	ambient_malloc_init(mf);

	if (speed) {
		runHashSpeedTests();
		runMacSpeedTests(random_supply);
		runAsymKeyGenSpeedTests(random_supply);
		runSymEncSpeedTests();
		runAsymEncSpeedTests(random_supply);
		runSigSpeedTests(random_supply);
	} else {
		int results = runDomainNameTests();
		if (results > 0) {
			cout << "Failed " << results << " DomainName tests!\n";
		} else {
			cout << "Passed all DomainName tests!\n";
		}

		results = runRecordTests(random_supply);
		if (results > 0) {
			cout << "Failed " << results << " Record tests!\n";
		} else {
			cout << "Passed all Record tests!\n";
		}

		results = runCertTests(random_supply);
		if (results > 0) {
			cout << "Failed " << results << " Cert tests!\n";
		} else {
			cout << "Passed all Cert tests!\n";
		}

		results = runKeyTests(random_supply);
		if (results > 0) {
			cout << "Failed " << results << " Key tests!\n";
		} else {
			cout << "Passed all Key tests!\n";
		}

		results = runRandTests();
		if (results > 0) {
			cout << "Failed " << results << " RandomSupply tests!\n";
		} else {
			cout << "Passed all random tests!\n";
		}
		
		results = runSymKeyTests(random_supply);
		if (results > 0) {
			cout << "Failed " << results << " SymKey tests!\n";
		} else {
			cout << "Passed all SymKey tests!\n";
		}

	}
	
		
	return result;
}


uint32_t getCurrentTime() {
	return (uint32_t) time (NULL);
}
