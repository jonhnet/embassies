#include "EncryptedKeyValClient.h"
#include "LiteLib.h"
#include "hash_table.h"
#include "zemit.h"

// Warning: Currently very copy happy
// TODO this file reproduces a lot of what's in KeyVal; it should be layered
// on a KeyVal instance instead.

EncryptedKeyValClient::EncryptedKeyValClient(MallocFactory* mf, AbstractSocket* sock, UDPEndpoint* server, RandomSupply* random_supply, KeyDerivationKey* appKey, ZLCEmit *ze)
  : KeyValClient(mf, sock, server, ze) {
	this->random_supply = random_supply;
	uint8_t label[] = "EncryptedKeyValClient auth enc key";
	this->key = appKey->deriveEncKey(label, sizeof(label));
}

EncryptedKeyValClient::~EncryptedKeyValClient() {
	delete this->key;
}

bool EncryptedKeyValClient::insert(KeyVal* kv) {
	// Wrap the entire kv, so we can check length and key on way out
	Ciphertext* cipher = this->key->encrypt(random_supply,
	                                        (uint8_t*)kv, 
											sizeof(KeyVal) + kv->val.len);

	int cipher_size = sizeof(Ciphertext) + cipher->len;
	int req_size = sizeof(KVRequest) + cipher_size; 
	KVRequest* req = (KVRequest*)mf_malloc(this->mf, req_size);

// TODO just a (perf-costing) debugging assertion -- can we
// decrypt what we just wrote?
#if 0
	{
		Plaintext* plain = this->key->decrypt(cipher);
		lite_assert(plain!=NULL);
		lite_assert(plain->len==sizeof(KeyVal) + kv->val.len);
		lite_assert(lite_memcmp(kv, plain->bytes, plain->len)==0);
		uint32_t cipher131hash = hash_buf(cipher, cipher_size);
		ZLC_COMPLAIN(ze, "Writing decodable cipher len %d hash 0x%x\n",,
			cipher_size, cipher131hash);
		char strbuf[80];
		int stridx = 0;
		for (int i=0; i<cipher_size; i++)
		{
			cheesy_snprintf(strbuf+stridx, sizeof(strbuf)-stridx, "%02x",
				((uint8_t*)cipher)[i]);
			stridx += 2;
			if (((i+1)%32)==0 || i==cipher_size-1)
			{
				strbuf[stridx] = '\0';
				ZLC_COMPLAIN(ze, "Bytes: %s\n",, strbuf);
				stridx = 0;
			}
		}
	}
#endif

	req->type = TYPE_INSERT;
	req->kv.key = kv->key;
	req->kv.val.len = cipher_size;

	lite_memcpy(req->kv.val.bytes, cipher, cipher_size);

	uint32_t hash131 = hash_buf(req->kv.val.bytes, req->kv.val.len);
#define READBACK_CHECK 1
#if READBACK_CHECK
	{
		ZLC_TERSE(ze, "Insert val len %d hash 0x%x\n",, req->kv.val.len, hash131);
	}
#endif // READBACK_CHECK

	bool rc = this->sock->sendto(this->server, req, req_size);
	if (!rc) { return false; }

	// Wait for and insist upon an acknowledgement of this message.
	ZeroCopyBuf* reply = this->sock->recvfrom(this->server);
	lite_assert(reply!=NULL);
	lite_assert(reply->len()==sizeof(uint32_t));
	lite_assert(((uint32_t*)reply->data())[0] == hash131);
	delete reply;

	mf_free(this->mf, req);
	delete cipher;
	return true;
}

Value* EncryptedKeyValClient::lookup(Key* key) {
	KVRequest req;

	req.type = TYPE_LOOKUP;
	req.kv.key = *key;

	bool rc = this->sock->sendto(this->server, &req, sizeof(KVRequest));
	if (!rc) { return NULL; }
	
	ZeroCopyBuf* buf = this->sock->recvfrom(this->server);
	if (buf == NULL) {
		return NULL;
	} else if (buf->len() < sizeof(Value)) {
		delete buf;
		return NULL;
	}

	Value* val = (Value*) buf->data();
	if (val->len == 0 || val->len > buf->len() || val->len < sizeof(Ciphertext)) {
		delete buf;
		return NULL;
	}

#if READBACK_CHECK
	{
		uint32_t bytes131hash = hash_buf(val->bytes, val->len);
		ZLC_TERSE(ze, "lookup val len %d hash 0x%x\n",, val->len, bytes131hash);
	}
#endif // READBACK_CHECK

	Ciphertext* cipher = (Ciphertext*) val->bytes;
	if (cipher->len == 0 || cipher->len > val->len) {
		delete buf;
		return NULL;
	}

	Plaintext* plain = this->key->decrypt(cipher);

	if (plain == NULL) {	// Decryption failed.  Panic! 
		delete buf;
		return NULL;
	}
	if (plain->len < sizeof(KeyVal)) {
		delete buf;
		delete plain;
		return NULL;
	}

	KeyVal* kv = (KeyVal*)plain->bytes;
	if (kv->key != *key) {	// KV we got doesn't match what we asked for!
		delete buf;
		delete plain;
		return NULL;
	}

	Value* new_val = (Value*) mf_malloc(this->mf, sizeof(Value)+kv->val.len);
	lite_memcpy(new_val, &kv->val, sizeof(Value)+kv->val.len);

	delete buf;
	delete plain;

	return new_val;
}

