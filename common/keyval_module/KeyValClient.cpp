#include "KeyValClient.h"
#include "LiteLib.h"
#include "zemit.h"
#include "hash_table.h"	// for debugging.

// Warning: Currently very copy happy

KeyValClient::KeyValClient(MallocFactory* mf, AbstractSocket* sock, UDPEndpoint* server, ZLCEmit *ze) {
	this->mf = mf;
	this->sock = sock;
	this->server = server;
	this->ze = ze;
}

KeyValClient::~KeyValClient() {
	mf_free(this->mf, this->server);
	delete this->sock;
}

bool KeyValClient::insert(KeyVal* kv) {
	int req_size = sizeof(KVRequest) + kv->val.len;
	KVRequest* req = (KVRequest*)mf_malloc(this->mf, req_size);

	req->type = TYPE_INSERT;
	req->kv = *kv;

	uint32_t hash131 = hash_buf(kv->val.bytes, kv->val.len);
#define READBACK_CHECK 1
#if READBACK_CHECK
	ZLC_TERSE(ze, "Insert val len %d hash 0x%x\n",, kv->val.len, hash131);
#endif // READBACK_CHECK

	lite_memcpy(req->kv.val.bytes, kv->val.bytes, kv->val.len);

	bool rc = this->sock->sendto(this->server, req, req_size);
	if (!rc) { return false; }

	ZeroCopyBuf* reply = this->sock->recvfrom(this->server);
	lite_assert(reply!=NULL);
	lite_assert(reply->len()==sizeof(uint32_t));
	lite_assert(((uint32_t*)reply->data())[0] == hash131);
	delete reply;

	mf_free(this->mf, req);
	return true;
}

Value* KeyValClient::lookup(Key* key) {
	KVRequest req;

	req.type = TYPE_LOOKUP;
	req.kv.key = *key;

	bool rc = this->sock->sendto(this->server, &req, sizeof(KVRequest));
	if (!rc) { return NULL; }
	
	ZeroCopyBuf* buf = this->sock->recvfrom(this->server);
	if (buf == NULL) {
		ZLC_TERSE(ze, "WARNING: timeout contacting KeyVal server\n");
		return NULL;
	}

	Value* val = (Value*) buf->data();

#if READBACK_CHECK
{
	uint32_t bytes131hash = hash_buf(val->bytes, val->len);
	ZLC_TERSE(ze, "lookup val len %d hash 0x%x\n",, val->len, bytes131hash);
}
#endif // READBACK_CHECK

	Value* new_val = (Value*) mf_malloc(this->mf, sizeof(Value)+val->len);
	lite_memcpy(new_val, val, sizeof(Value)+val->len);
	delete buf;

#if READBACK_CHECK
{
	uint32_t bytes131hash = hash_buf(new_val->bytes, new_val->len);
	ZLC_TERSE(ze, "lookup new_val len %d hash 0x%x\n",, new_val->len, bytes131hash);
}
#endif // READBACK_CHECK

	return new_val;
}

