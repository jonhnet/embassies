#include <fstream>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "MonitorCrypto.h"
#include "zoog_root_key.h"	// makefile special

#define APP_MASTER_KEY_DIR 			ZOOG_ROOT "/monitors/build"
#define APP_MASTER_KEY_FILE_NAME	APP_MASTER_KEY_DIR "/app_master.key"

using namespace std;

MonitorCrypto::MonitorCrypto(KeyPairRequest kpr)
{
	ifstream urand;
	urand.open("/dev/urandom", ios::binary);
	assert(!urand.fail());
	uint8_t buffer[RandomSupply::SEED_SIZE];
	urand.read((char*)buffer, sizeof(buffer));
	urand.close();
	this->random_supply = new RandomSupply(buffer);

	// We only need the public key, so don't hold onto the pair
	// TODO: Update keypairs so we can grab only the public part directly
	ZKeyPair* rootKeyPair = new ZKeyPair((uint8_t*)zoog_root_key_bytes, sizeof(zoog_root_key_bytes));
	zoog_ca_root_key = new ZPubKey(rootKeyPair->getPubKey());
	delete rootKeyPair;

	// Try to read in an existing app master key, or else create one
	//  TODO: This leaks the key, since there doesn't appear to be a Monitor destructor
	ifstream app_master_key_file;
	app_master_key_file.open((char*)APP_MASTER_KEY_FILE_NAME, ios::binary);
	if (!app_master_key_file.fail()) {
		// TODO: Get the actual file size, instead of default key size
		uint8_t key_bytes[SYM_KEY_BYTES];
		app_master_key_file.read((char*)key_bytes, sizeof(key_bytes));
		app_master_key_file.close();
		this->app_master_key = new KeyDerivationKey(key_bytes, sizeof(key_bytes));
	} else {
		this->app_master_key = new KeyDerivationKey(this->random_supply);

		mkdir(APP_MASTER_KEY_DIR, 0755);

		ofstream file;
		file.open(APP_MASTER_KEY_FILE_NAME, ios::binary);
		if (file.fail()) {
			assert(false);	// Failed to write out master key!
		}
		file.write((char*)this->app_master_key->getBytes(), 
		           this->app_master_key->getNumBytes());
		file.close();
	}

	if (kpr == WANT_MONITOR_KEY_PAIR)
	{
		this->monitorKeyPair = new ZKeyPair(this->random_supply);
	}
	else
	{
		this->monitorKeyPair = NULL;
	}
}
