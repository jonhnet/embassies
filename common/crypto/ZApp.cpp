
#include <iostream>
#include <fstream>
#include <stdio.h>

#ifdef _WIN32
#define snprintf(a, b, c, d) _snprintf_s(a, sizeof(a), b, c, d)
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#endif // _WIN32

#include "args_lib.h"
#include "ZApp.h"
#include "ZKeyPair.h"
#include "ZCert.h"
#include "CryptoException.h"
#include "tests.h"

using namespace std;

#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 1024
#define MAX_KEY_PAIR_SIZE 8096


// Read a file (up to size bytes) into the buffer provided.  
//static void readFile(char* filename, ByteStream &buffer) {
static void readFile(char* filename, uint8_t* buffer, uint32_t size) {
	ifstream file;

	file.open(filename, ios::binary);
	if (!file.is_open()) {
		cout << "Unable to open file " << filename << " for reading\n";
		exit(1);
	}

	//buffer.read(file);
	file.read((char*)buffer, size);
	file.close();
}

// Write the entire buffer into a file.  
// Overwrites the file if it exists
static void writeFile(char* filename, uint8_t* buffer, uint32_t size) {
//static void writeFile(char* filename, ByteStream &buffer) {
	ofstream file;
	file.open(filename, ios::binary);
	if (!file.is_open()) {
		cout << "Unable to open file " << filename << " for writing\n";
		exit(1);
	}

	//buffer.write(file);
	file.write((char*)buffer, size);
	file.close();
}

// Hold onto info about a file mapping
#ifdef _WIN32
typedef struct {
	uint8_t* start;
	uint32_t size;
	HANDLE fileHandle;
	HANDLE mappingHandle;
} FileMapping;
#else  // !_WIN32
typedef struct {
	uint8_t* start;
	uint32_t size;
	int fd;
} FileMapping;
#endif

// Given a file name, creates a file mapping with relevant info in the FileMapping struct
static void mmapFile(char* filename, FileMapping* mapping) {
#ifdef _WIN32
	mapping->fileHandle = CreateFile(filename, 
									GENERIC_READ, 
									FILE_SHARE_READ | FILE_SHARE_DELETE, 
									NULL, 
									OPEN_EXISTING, 
									FILE_ATTRIBUTE_NORMAL, 
									NULL);
	if (mapping->fileHandle == INVALID_HANDLE_VALUE) {
		Throw(CryptoException::BAD_INPUT, "Unable to open file to map", GetLastError());
	}

	// Create the mapping
	mapping->mappingHandle = CreateFileMapping(mapping->fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if (mapping->mappingHandle == NULL) {
		CloseHandle(mapping->fileHandle);
		Throw(CryptoException::BAD_INPUT, "Unable to create a map for file", GetLastError());
	}

	// Actually map the file 
	mapping->start = (uint8_t*) MapViewOfFile(mapping->mappingHandle, FILE_MAP_READ, 0, 0, 0);
	if (!mapping->start) {
		CloseHandle(mapping->mappingHandle);
		CloseHandle(mapping->fileHandle);
		Throw(CryptoException::BAD_INPUT, "Unable to map the file", GetLastError());
	}

	// Figure out how big it is
	mapping->size = GetFileSize(mapping->fileHandle, NULL);	// Use the second argument if fileSize needs 64 bits

#else  // !_WIN32
    mapping->fd = open(filename, O_RDONLY);
    if (mapping->fd == -1) {
        Throw(CryptoException::BAD_INPUT, "Unable to open file to map");
    }

    // Figure out the file's size
    struct stat statBuffer;
    if (fstat(mapping->fd, &statBuffer) == -1) {
        close(mapping->fd);
        Throw(CryptoException::BAD_INPUT, "Unable to get file size for mapping");
    }

    mapping->size = statBuffer.st_size;

    // Actually map the file
    mapping->start = (uint8_t*)mmap(NULL, mapping->size, PROT_READ, MAP_PRIVATE, mapping->fd, 0);
    if (mapping->start == MAP_FAILED) {
        close(mapping->fd);
        Throw(CryptoException::BAD_INPUT, "Unable to map the file");

    }
#endif // _WIN32
}

static void unMapFile(FileMapping* mapping) {
#ifdef _WIN32			
	UnmapViewOfFile(mapping->start);
	CloseHandle(mapping->mappingHandle);
	CloseHandle(mapping->fileHandle);
#else  // !_WIN32
	munmap(mapping->start, mapping->size);
	close(mapping->fd);
#endif // _WIN32
}

void ZApp::printUsage() {
	cout << "Expected arguments: \n";
	cout << "\tMust include exactly one of the following:\n";
	cout << "\t --speed true \t\t Test the speed of various cryptographic primitives\n";
	cout << "\t --test true\t\t Test the crypto library's functionality, checking for regression bugs\n";
	cout << "\t --genkeys [filename] \t\t Generate a keypair and write it to the file specified\n";
	cout << "\t --genchain [filename] [cert1] [cert2] ... \t\t Compact certs into a chain and write to filename\n";
	cout << "\t --binary [bin-file] \t\t Binary to sign\n";
	cout << "\t --ekeypair [key-file] \t\t Key pair to endorse\n";
	cout << "\t --ekeylink \t\t Endorse key1 speaksfor key2\n";
	cout << "\t --verify [certfile] \t\t Verify the specified certificate\n";
	cout << "\tFor --genkeypair, you may include:\n";
	cout << "\t --name [domainName]\t\t Name to associate with the key pair\n";
	cout << "\tFor --binary, --ekeypair, and --ekeylink, you must include the following:\n";
	cout << "\t --cert [filename] \t\t Resulting certificate will be written here\n";	
	cout << "\t --skeypair [key-file] \t\t Key pair to sign the cert with\n";
	cout << "\t --inception [unixTime]\t\t Certificate is valid starting on this date\n";
	cout << "\t --expires [unixTime]\t\t Expiration date for this certificate\n";
	cout << "\tFor --ekeypair, you must also include:\n";
	cout << "\t --name [domainName]\t\t Name to associate with the key being certified\n";
	cout << "\tFor --ekeylink, you must also include:\n";
	cout << "\t --speaker [key-file]\t\t This key speaks for the spoken key\n";
	cout << "\t --spoken [key-file]\t\t This key is spoken for by speaker\n";
	cout << "\tFor the last one (--verify), you must include:\n";
	cout << "\t --vkeypair [key-file] \t\t Key pair with which to validate the certificate\n";		
}

// Generate a fresh keypair with an optional name associated
// Write the keypair to the file provided
static void generateKeyPair(RandomSupply* random_supply, char* filename, char* name) {
	ZKeyPair* keys = NULL;

	if (name == NULL) {
		keys = new ZKeyPair(random_supply);
	} else {
		DomainName* dname = new DomainName(name, strlen(name)+1);
		keys = new ZKeyPair(random_supply, dname);
		delete dname;
	}
			
	uint8_t* buffer = new uint8_t[keys->size()];
	keys->serialize(buffer);
	writeFile(filename, buffer, keys->size());
		
	if (keys->size() > MAX_KEY_PAIR_SIZE) {
		cout << "Warning!  These keys are larger than expected.  We'll have trouble reading them back in!\n";
		cout << "Try increasing MAX_KEY_PAIR_SIZE\n";
	}

	delete [] buffer;
	delete keys;			
}

static ZCert* prepareBinaryCert(char* binaryFilename, uint32_t inception, uint32_t expires) {
	// Map the binary file into memory
	FileMapping mapping;

	mmapFile(binaryFilename, &mapping);

	// Prep the cert
	ZCert* cert = new ZCert(inception, expires, mapping.start, mapping.size);
	assert(cert);

	// Cleanup the binary
	unMapFile(&mapping);

	return cert;
}

static ZCert* prepareKeyCert(char* ekeypairFilename, DomainName* name, uint32_t inception, uint32_t expires) {
	// Read in the key we're going to sign
	uint8_t keybuffer[MAX_KEY_PAIR_SIZE];
	readFile(ekeypairFilename, keybuffer, MAX_KEY_PAIR_SIZE);
	ZKeyPair* ekeys = new ZKeyPair(keybuffer, MAX_KEY_PAIR_SIZE);
	assert(ekeys);
	
	ZCert* cert = new ZCert(inception, expires, name, ekeys->getPubKey());
	assert(cert);

	delete ekeys;

	return cert;
}

static ZCert* prepareKeyLinkCert(char* speakerFilename, char* spokenFilename, uint32_t inception, uint32_t expires) {
	// Read in the keys we're going to sign
	uint8_t keybuffer[MAX_KEY_PAIR_SIZE];
	
	readFile(speakerFilename, keybuffer, MAX_KEY_PAIR_SIZE);
	ZKeyPair* speaker = new ZKeyPair(keybuffer, MAX_KEY_PAIR_SIZE);
	assert(speaker);

	readFile(spokenFilename, keybuffer, MAX_KEY_PAIR_SIZE);
	ZKeyPair* spoken = new ZKeyPair(keybuffer, MAX_KEY_PAIR_SIZE);
	assert(spoken);

	ZCert* cert = new ZCert(inception, expires, speaker->getPubKey(), spoken->getPubKey());

	delete speaker;
	delete spoken;

	return cert;
}

// Verify the cert using the specified keypair
static void verifyCert(char* vCertFilename, char* vkeypairFilename) {
	// Load the cert
	uint8_t certBytes[ZCert::MAX_CERT_SIZE];
	readFile(vCertFilename, certBytes, ZCert::MAX_CERT_SIZE);			
	ZCert* cert = new ZCert(certBytes, ZCert::MAX_CERT_SIZE);
	assert(cert);

	// Load the verification key
	uint8_t keybuffer[MAX_KEY_PAIR_SIZE];
	readFile(vkeypairFilename, keybuffer, MAX_KEY_PAIR_SIZE);			
	ZKeyPair* keys = new ZKeyPair(keybuffer, MAX_KEY_PAIR_SIZE);
	assert(keys);

	if (cert->verify(keys->getPubKey())) {
		cout << "Yep, that's a good cert!\n";
	} else {
		cout << "That's a bad cert!  What are you trying to pull?!\n";
	}
}

void ZApp::run(RandomSupply* random_supply, int argc, char **argv) {	
	// Skip the executable name
	argc-=1; argv+=1;

	if (argc < 2) { 
		printUsage();
		exit(1);
	}

	//char gkeypairFilename[MAX_PATH_LENGTH], binaryFilename[MAX_PATH_LENGTH], ekeypairFilename[MAX_PATH_LENGTH];
	//char certFilename[MAX_PATH_LENGTH], skeypair[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
	char* gkeypairFilename = NULL;
	char* binaryFilename = NULL; 
	char* ekeypairFilename = NULL;
	bool ekeylinkPresent = false;
	char* certFilename = NULL;
	char* skeypairFilename = NULL;
	char* vCertFilename = NULL;
	char* speakerFilename = NULL;
	char* spokenFilename = NULL;
	char* vkeypairFilename = NULL;
	char* name = NULL;
	uint32_t inception = 0, expires = 0;
	bool speedTest = false;
	bool functionalityTest = false;
	char* chainFilename = NULL;

	while (argc>0) {
		if (chainFilename != NULL) {
			break;
		}

		CONSUME_STRING("--genkeys", gkeypairFilename);
		CONSUME_STRING("--binary", binaryFilename);
		CONSUME_STRING("--ekeypair", ekeypairFilename);
		CONSUME_OPTION("--ekeylink", ekeylinkPresent);
		CONSUME_STRING("--verify", vCertFilename);
		CONSUME_STRING("--cert", certFilename);
		CONSUME_STRING("--skeypair", skeypairFilename);
		CONSUME_STRING("--name", name);
		CONSUME_INT("--inception", inception);
		CONSUME_INT("--expires", expires);
		CONSUME_STRING("--speaker", speakerFilename);
		CONSUME_STRING("--spoken", spokenFilename);
		CONSUME_STRING("--vkeypair", vkeypairFilename);
		CONSUME_BOOL("--speed", speedTest);
		CONSUME_BOOL("--test", functionalityTest);
		CONSUME_STRING("--genchain", chainFilename);

		cout << "Unknown argument: '" << argv[0] << "'\n";
		exit(1);
	}

	try {
		if (speedTest) {
			runTests(random_supply, true);
		} else if (functionalityTest) {
			runTests(random_supply, false);
		} else if (gkeypairFilename != NULL) {
			generateKeyPair(random_supply, gkeypairFilename, name);
		} else if (chainFilename != NULL) {
			ZCertChain chain; 

			for (int i = 0; i < argc; i++) {
				// Load the cert
				uint8_t certBytes[ZCert::MAX_CERT_SIZE];
				readFile(argv[i], certBytes, ZCert::MAX_CERT_SIZE);			
				ZCert* cert = new ZCert(certBytes, ZCert::MAX_CERT_SIZE);

				if (!cert) {
					cout << "Failed to load cert from " << argv[i] << endl;
					exit(1);
				}

				chain.addCert(cert);
			}
			
			uint8_t* buffer = new uint8_t[chain.size()];
			assert(buffer);

			chain.serialize(buffer);
			writeFile(chainFilename, buffer, chain.size());
		
			for (uint32_t i=0; i < chain.getNumCerts(); i++) {
				delete chain[i];
			}
		} else if (binaryFilename != NULL || ekeypairFilename != NULL || ekeylinkPresent) {
			if (!certFilename || !skeypairFilename || expires == 0 || inception > expires) {
				printUsage();
				cout << "Missing required argument for certification!\n";
				exit(1);
			}

			ZCert* cert = NULL;

			// Prepare an appropriate certificate
			if (binaryFilename != NULL) { // We're signing a binary			
				cert = prepareBinaryCert(binaryFilename, inception, expires);
			} else if (ekeypairFilename != NULL) {	// We're signing a cert binding a name to a key
				if (!name) {
					printUsage();
					cout << "Missing required argument for key certification!\n";
					exit(1);
				}
				DomainName* dname = new DomainName(name, strlen(name) + 1);
				cert = prepareKeyCert(ekeypairFilename, dname, inception, expires);
				delete dname;
			} else { // We're signing a link between two keys
				if (!speakerFilename || !spokenFilename) {
					printUsage();
					cout << "Missing required argument for key link certification!\n";
					exit(1);
				}
				cert = prepareKeyLinkCert(speakerFilename, spokenFilename, inception, expires);
			}

			// Get the key pair we're going to sign with
			uint8_t keybuffer[MAX_KEY_PAIR_SIZE];
			readFile(skeypairFilename, keybuffer, MAX_KEY_PAIR_SIZE);
			ZKeyPair* skeys = new ZKeyPair(keybuffer, MAX_KEY_PAIR_SIZE);

			// Sign the cert
			cert->sign(skeys->getPrivKey());

			// Write it out
			uint8_t* buffer = new uint8_t[cert->size()];
			cert->serialize(buffer);
			writeFile(certFilename, buffer, cert->size());

			if (cert->size() > ZCert::MAX_CERT_SIZE) {
				cout << "Warning!  This cert is larger than expected.  We'll have trouble reading it back in!\n";
				cout << "Try increasing ZCert::MAX_CERT_SIZE\n";
			}

			delete [] buffer;
			delete cert;
			delete skeys;		
		} else if (vCertFilename != NULL) {
			if (vkeypairFilename == NULL) {
				cout << "Missing keypair argument!\n";
				printUsage();
				exit(1);
			}

			verifyCert(vCertFilename, vkeypairFilename);
		} else {
			printUsage();
		}
	} catch (CryptoException e) {
		cout << "Caught an exception! " << e << endl;
	}
}
