#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "hash_table.h"
#include "xax_network_utils.h"
#include "xax_util.h"
#include "standard_malloc_factory.h"
#include "xax_util.h"

#include "KeyVal.h"


// Implements DJBX33X: Treats key as a character string.
// At each stage, multiply hash by 33 and xor in a character
uint32_t hash(const void *datum) {
	KeyVal* kv = (KeyVal*) datum;
	Key* key = &kv->key;
	
	uint32_t hash = 5381;
	for (unsigned int i = 0; i < sizeof(Key); i++) {
		hash = ((hash << 5) + hash) ^ ((uint8_t*)&key)[i];
	}
	return hash;
}

int cmp(const void* a, const void* b) {
	KeyVal* ka = (KeyVal*)a;
	KeyVal* kb = (KeyVal*)b;

	if (ka->key < kb->key) {
		return -1;
	} else if (ka->key == kb->key) {
		return 0;
	} else {	// ka > kb
		return 1;
	}
}

KeyVal* KV_clone(KeyVal* kv) {
	KeyVal* ret = (KeyVal*)malloc(sizeof(KeyVal) + kv->val.len);
	*ret = *kv;
	memcpy(ret->val.bytes, kv->val.bytes, kv->val.len);
	return ret;
}

class KVServer {
private:
	struct sockaddr_in server_addr, remote_addr;
	int sock;
	HashTable db;
	uint8_t buffer[MAX_KV_SIZE];

	void annotate(const char *label, KeyVal* kv);
	void handle_lookup(KVRequest* request);
	void handle_insert(KVRequest* request);

public:
	KVServer();
	void run();
};

KVServer::KVServer()
{
	// Initialize our hash table
	MallocFactory* mf = standard_malloc_factory_init();
	hash_table_init(&db, mf, hash, cmp);

	// Create the server's socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		fprintf(stderr, "(KeyValueServer) Failed to open server socket!\n");
		exit(1);
	}

	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KEYVAL_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		fprintf(stderr, "(KeyValueServer) Failed to bind server socket!\n");
		exit(1);
	}
}

void KVServer::annotate(const char *label, KeyVal* kv)
{
	uint32_t hash131 = hash_buf(kv->val.bytes, kv->val.len);
	fprintf(stderr, "\t%s: key %d len %d hash131 %08x\n",
		label, (int) kv->key, kv->val.len, hash131);
}

void KVServer::handle_lookup(KVRequest* request)
{
	int rc = 0;
	KeyVal* newKV = NULL;

	printf("Looking for key %d\n", (int)request->kv.key); fflush(stdout);
	newKV = (KeyVal*) hash_table_lookup(&db, &request->kv);
	if (newKV == NULL) {
		// Send an empty reply 
		printf("\tFailed to find key.  Sending empty reply.\n"); fflush(stdout);
		Value empty;
		empty.len = 0;
		rc = sendto(sock, &empty, sizeof(Value), 0, (sockaddr*)&remote_addr, sizeof(remote_addr));
	} else {
		annotate("found", newKV);
		rc = sendto(sock, &newKV->val, newKV->val.len+sizeof(Value), 0, (sockaddr*)&remote_addr, sizeof(remote_addr));
	}
	if (rc == -1) { fprintf(stderr, "(KeyValueServer) Failed to send lookup reply!\n"); }
}

void KVServer::handle_insert(KVRequest* request)
{
	KeyVal* newKV = NULL;

	// If the key is already present, we need to remove it first
	// (because hash_table doesn't like inserts for existing values)
	newKV = (KeyVal*) hash_table_lookup(&db, &request->kv);
	if (newKV != NULL) {
		hash_table_remove(&db, newKV);
		free(newKV);
	}

	// Now copy the request and insert it into our database
	newKV = KV_clone(&request->kv);
	uint32_t hash131 = hash_buf(newKV->val.bytes, newKV->val.len);
	lite_assert(newKV);
	hash_table_insert(&db, newKV);
	annotate("insert", newKV);

	int rc;
	rc = sendto(sock, &hash131, sizeof(hash131), 0, (sockaddr*)&remote_addr, sizeof(remote_addr));
	if (rc == -1) { fprintf(stderr, "(KeyValueServer) Failed to send insert reply!\n"); }
}

void KVServer::run()
{
	while (1) {
		socklen_t remote_addr_len = sizeof(remote_addr);
		printf("Waiting for a packet ...\n");
		fflush(stdout);
		int rc = recvfrom(sock, buffer, MAX_KV_SIZE, 0, (sockaddr*)&remote_addr, &remote_addr_len);
		if (-1 == rc) {
			fprintf(stderr, "(KeyValueServer) Failed to receive packet!\n");
			exit(1);
		}
		lite_assert(rc<MAX_KV_SIZE);	// darn -- MAX_KV_SIZE too small for actual usage.

		lite_assert(rc>=(int)sizeof(KVRequest));
		KVRequest* request = (KVRequest*)buffer;

		switch (request->type) {
			case TYPE_LOOKUP:
				handle_lookup(request);
				break;
			case TYPE_INSERT:
				handle_insert(request);
				break;
			default:
				fprintf(stderr, "(KeyValueServer) Dropping malformed request with type %04x.\n", request->type);
				continue;
		}
	}

	// TODO:Doesn't clean-up after itself yet
	hash_table_free_discard(&db);
}


int main() {
	KVServer server;
	server.run();
}
