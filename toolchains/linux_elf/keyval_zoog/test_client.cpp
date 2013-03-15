#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "KeyVal.h"

//#define SERVER_IP "10.4.0.2"
//#define SERVER_IP "127.0.0.1"
//#define SERVER_IP "10.4.255.255"
//#define SERVER_IP "255.255.255.255"
#define SERVER_IP "10.198.235.187"

struct sockaddr_in server_addr;
int sock = 0;

void insert(KeyVal* kv) {
	int req_size = sizeof(KVRequest) + kv->val.len;
	KVRequest* req = (KVRequest*)malloc(req_size);

	req->type = TYPE_INSERT;
	req->kv = *kv;

	memcpy(req->kv.val.bytes, kv->val.bytes, kv->val.len);

	int rc = sendto(sock, req, req_size, 0, (sockaddr*)&server_addr, sizeof(server_addr));
	if (rc == -1) { fprintf(stderr, "(KeyValueTestClient) Failed to send insert request!\n"); }
}

Value* lookup(Key* key) {
	KVRequest req;

	req.type = TYPE_LOOKUP;
	req.kv.key = *key;

	int rc = sendto(sock, &req, sizeof(KVRequest), 0, (sockaddr*)&server_addr, sizeof(server_addr));
	if (rc == -1) { fprintf(stderr, "(KeyValueTestClient) Failed to send lookup request!\n"); }
	
	uint8_t buffer[MAX_KV_SIZE];
	rc = recv(sock, buffer, MAX_KV_SIZE, 0);

	if (rc == -1) {
		fprintf(stderr, "(KeyValueTestClient) Failed to receive packet!\n");
		exit(1);
	}

	Value* val = (Value*) malloc(sizeof(Value)+rc);
	memcpy(val, buffer, sizeof(Value) + ((Value*)buffer)->len);

	return val;
}

int main() {
	// Create the client's socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		fprintf(stderr, "(KeyValueTestClient) Failed to open socket!\n");
		exit(1);
	}

	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KEYVAL_PORT);
	if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0) {
		fprintf(stderr, "(KeyValueTestClient) Failed to convert IP address!\n");
		exit(1);
	}
	
	uint8_t buffer[MAX_KV_SIZE];
	printf("Sending a packet ...\n");
	fflush(stdout);

	// Lookup a non-existent key
	Key key = 7;
	Value* val = lookup(&key);

	if (val->len > 0) {
		fprintf(stderr, "(KeyValueTestClient) Got non-zero answer to non-existent key!\n");
		exit(1);
	}
	free(val);
	printf("Passed empty lookup test\n");

	// Insert an item
	KeyVal* kv = (KeyVal*)buffer;
	kv->key = 42;
	kv->val.len = 2;
	kv->val.bytes[0] = 'B';
	kv->val.bytes[1] = 'P';

	insert(kv);
	printf("Inserted key 42\n");

	// Lookup the item we just inserted
	key = 42;
	val = lookup(&key);

	if (val->len != 2) {
		fprintf(stderr, "(KeyValueTestClient) Lookup returned length of %d, not 2!\n", val->len);
		exit(1);
	}

	if (val->bytes[0] != 'B' || val->bytes[1] != 'P') {
		fprintf(stderr, "(KeyValueTestClient) Lookup bytes wrong!\n");
		exit(1);
	}
	free(val);
	printf("Found item with key 42\n");

	// Try a lookup on a non-existant key again
	key = 7;
	val = lookup(&key);

	if (val->len > 0) {
		fprintf(stderr, "(KeyValueTestClient) Got non-zero answer to non-existent key!\n");
		exit(1);
	}
	free(val);
	printf("Passed empty lookup test with key 7\n");

	// Insert a second item
	kv->key = 21;
	kv->val.len = 5;
	kv->val.bytes[0] = 'B';
	kv->val.bytes[1] = 'R';
	kv->val.bytes[2] = 'Y';
	kv->val.bytes[3] = 'A';
	kv->val.bytes[4] = 'N';

	insert(kv);
	printf("Inserted key 21\n");

	// Lookup the item we just inserted
	key = 21;
	val = lookup(&key);

	if (val->len != 5) {
		fprintf(stderr, "(KeyValueTestClient) Lookup returned length of %d, not 5!\n", val->len);
		exit(1);
	}

	if (val->bytes[0] != 'B' || 
			val->bytes[1] != 'R' || 
			val->bytes[2] != 'Y' || 
			val->bytes[3] != 'A' || 
			val->bytes[4] != 'N') {
		fprintf(stderr, "(KeyValueTestClient) Lookup bytes wrong!\n");
		exit(1);
	}
	free(val);
	printf("Found key 21\n");

	// And lookup the item we first inserted
	key = 42;
	val = lookup(&key);

	if (val->len != 2) {
		fprintf(stderr, "(KeyValueTestClient) Lookup returned length of %d, not 2!\n", val->len);
		exit(1);
	}

	if (val->bytes[0] != 'B' || val->bytes[1] != 'P') {
		fprintf(stderr, "(KeyValueTestClient) Lookup bytes wrong!\n");
		exit(1);
	}
	free(val);
	printf("Still found key 42\n");

	printf("\nPassed all key-value tests!\n"); fflush(stdout);
}


