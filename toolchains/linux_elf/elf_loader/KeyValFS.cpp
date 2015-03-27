#include <errno.h>

#include "KeyValFS.h"
#include "SocketFactory_Skinny.h"
#include "xax_network_defs.h"
#include "LiteLib.h"
#include "ZLCEmitXdt.h"
#include "EncryptedKeyValClient.h"
#include "perf_measure.h"

// TODO: Probably not thread safe, since everyone shares the same socket

extern uint32_t insecure_hash(const char* key, const uint32_t key_len);

KeyValFSHandle::KeyValFSHandle(KeyValClient* kv_client, Key* key, PerfMeasure* perf) {
	this->kv_client = kv_client;
	this->key = key;
	this->perf_measure = perf;
}

// Grabs the old value and writes back "length" bytes of it, 
// extending with 0s if necessary
void KeyValFSHandle::ftruncate(
			XfsErr *err, int64_t length) {
	Value* val = this->kv_client->lookup(this->key);
	lite_assert(val);

	KeyVal* kv = (KeyVal*) mf_malloc(this->kv_client->get_mf(), sizeof(KeyVal) + length);
	kv->key = *this->key;
	kv->val.len = length;

	// Copy over the relevant bytes of the old value
	lite_memcpy(kv->val.bytes, val->bytes, val->len);

	// Extend with 0s if necessary
	if (length > val->len) {
		lite_memset(kv->val.bytes+val->len, 0, length - val->len);
	}

	this->kv_client->insert(kv);

	mf_free(this->kv_client->get_mf(), kv);
	mf_free(this->kv_client->get_mf(), val);

	*err = XFS_NO_ERROR;
}

void KeyValFSHandle::read(
	XfsErr *err, void *dst, size_t len, uint64_t offset) {
	perf_measure->mark_time("read_keyvalfs start");
	Value* val = this->kv_client->lookup(this->key);
	lite_assert(val);
	lite_assert(len + offset <= val->len);
	uint32_t num_bytes = len;
	//uint32_t num_bytes = offset + len > val->len ? val->len - offset : len;
	lite_memcpy(dst, &val->bytes[offset], num_bytes);
	mf_free(this->kv_client->get_mf(), val);
	*err = XFS_NO_ERROR;
	perf_measure->mark_time("read_keyvalfs end");
}

void KeyValFSHandle::write(
    XfsErr *err, const void *src, size_t len, uint64_t offset) {
	perf_measure->mark_time("write_keyvalfs start");
	Value* val = this->kv_client->lookup(this->key);
	lite_assert(val);
	lite_assert(offset+len <= val->len);	// Can't write past end 

	// Duplicate value into a KV struct 
	// (TODO: Make KV contain pointers to avoid this)
	KeyVal* kv = (KeyVal*) mf_malloc(this->kv_client->get_mf(), sizeof(KeyVal) + val->len);
	kv->key = *this->key;
	kv->val.len = val->len;

	// Copy over the old value 
	lite_memcpy(kv->val.bytes, val->bytes, val->len);

	// Overwrite with new data
	lite_memcpy(kv->val.bytes+offset, src, len);

	this->kv_client->insert(kv);

	mf_free(this->kv_client->get_mf(), kv);
	mf_free(this->kv_client->get_mf(), val);

	*err = XFS_NO_ERROR;
	perf_measure->mark_time("write_keyvalfs end");
}

void KeyValFSHandle::xvfs_fstat64(XfsErr *err, struct stat64 *buf) {
	XfsErr dummy;
	xnull_fstat64(&dummy, buf, get_file_len());
	
	//static int fake_ino_supply = 1800;
	buf->st_dev = 17;
	//statbuf.st_ino = fake_ino_supply++;
	buf->st_ino = 1800;
	buf->st_atime = 1272062690;
	buf->st_mtime = 1272062690;
	buf->st_ctime = 1272062690;
	buf->st_mode = S_IFREG | S_IRUSR | S_IXUSR;

	*err = XFS_NO_ERROR;
}

bool KeyValFSHandle::is_stream() {
	return false;
}

uint64_t KeyValFSHandle::get_file_len() {
	Value* val = this->kv_client->lookup(this->key);
	uint64_t len = 0;
	if (val != NULL) {
		len = val->len;
		mf_free(this->kv_client->get_mf(), val);
	}
	return len;
}

KeyValFS::KeyValFS(XaxPosixEmulation* xpe, const char* path, bool encrypted) {
	this->mf = xpe->mf;
	this->perf_measure = xpe->perf_measure;
	
	XIPAddr broadcast = get_ip_subnet_broadcast_address(ipv4);
  UDPEndpoint* server_endpoint = make_UDPEndpoint(this->mf,  &broadcast, KEYVAL_PORT);
  lite_assert(server_endpoint);

	SocketFactory* socket_factory = new SocketFactory_Skinny(xpe->xax_skinny_network);
  AbstractSocket* server_sock = socket_factory->new_socket(
			socket_factory->get_inaddr_any(ipv4), NULL, true);
  lite_assert(server_sock);
	delete socket_factory;

	ZLCEmit *ze = new ZLCEmitXdt(xpe->zdt, terse);
	if (encrypted) {
		uint8_t seed[RandomSupply::SEED_SIZE];
		xpe->zdt->zoog_get_random(sizeof(seed), seed);
		RandomSupply* random_supply = new RandomSupply(seed);

		uint8_t app_secret[SYM_KEY_BITS/8];
		xpe->zdt->zoog_get_app_secret(sizeof(app_secret), app_secret);
		KeyDerivationKey* appKey = new KeyDerivationKey(app_secret, sizeof(app_secret));

		this->kv_client = new EncryptedKeyValClient(this->mf, server_sock, server_endpoint, random_supply, appKey, ze);
	} else {
		this->kv_client = new KeyValClient(this->mf, server_sock, server_endpoint, ze);
	}
	this->key = new Key;
	*this->key = insecure_hash(path, lite_strlen(path));
}

XaxVFSHandleIfc *KeyValFS::open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *xoh) {
	// Does the "file" already exist?
	Value* val = this->kv_client->lookup(this->key);
	if (val != NULL) {
		// File exists -- check that this is okay
		if (oflag & O_CREAT && oflag & O_EXCL) { // Nope
			*err = (XfsErr) EEXIST;
			return NULL;
		}
		mf_free(this->kv_client->get_mf(), val);
	} else if (oflag & O_CREAT) {
		// Create the file
		KeyVal kv;
		kv.key = *this->key;
		kv.val.len = 0;
		this->kv_client->insert(&kv);
	} else {
		// File doesn't exist, but create flag wasn't specified
		*err = (XfsErr)ENOENT;
		return NULL;
	}

	*err = XFS_NO_ERROR;
	return new KeyValFSHandle(this->kv_client, this->key, this->perf_measure);
}

KeyValFS *xax_create_keyvalfs(XaxPosixEmulation* xpe, const char* path, bool encrypted) {
	KeyValFS* kvfs = new KeyValFS(xpe, path, encrypted);

	xpe_mount(xpe, mf_strdup(xpe->mf, path), kvfs);
	return kvfs;
}
