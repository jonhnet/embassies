#include "ZFastFetch.h"
#include "ZSyntheticFileRequest.h"
#include "xax_network_utils.h"
#include "zftp_util.h"
#include "zlc_util.h"
#include "zftp_dir_format.h"


// Used to parcel out the hashing of a large data chunk
typedef struct {
	uint8_t* start;
	uint32_t num_bytes;
	hash_t* result;
	SyncFactoryEvent* event_done;
} hash_assign;

void parallel_hash(void* arg) {
	hash_assign* assign = (hash_assign*) arg;

	zhash(assign->start, assign->num_bytes, assign->result);

	assign->event_done->signal();
}

// Used to parcel out the MAC'ing of a large data chunk
typedef struct {
	uint8_t* start;
	uint32_t num_bytes;
	Mac* result;
	MacKey* key;
	RandomSupply* random_supply;
	SyncFactoryEvent* event_done;
} mac_assign;

// Used to parcel out the verification of a MAC of a large data chunk
typedef struct {
	uint8_t* start;
	uint32_t num_bytes;
	bool* result;
	MacKey* key;
	Mac* old_mac;
	SyncFactoryEvent* event_done;
} mac_verify_assign;


void parallel_mac_verify(void* arg) {
	mac_verify_assign* assign = (mac_verify_assign*) arg;

	*assign->result = assign->key->verify(assign->old_mac, 
	                                      assign->start, 
																				assign->num_bytes);

	assign->event_done->signal();
}

void parallel_mac(void* arg) {
	mac_assign* assign = (mac_assign*) arg;

	// Compute the MAC of this chunk and stick it in the buffer provided
	Mac* mac = assign->key->mac(assign->random_supply, 
	                            assign->start, 
															assign->num_bytes);
	memcpy(assign->result, mac, sizeof(Mac));
	delete mac;

	assign->event_done->signal();
}


void ZFastFetch::distribute_mac(uint8_t* start, uint32_t num_bytes, 
                                mac_list* result, RandomSupply* random_supply,
																MacKey* key) {
#if NUM_THREADS > 1
	mac_assign assignments[NUM_THREADS];

	uint32_t num_bytes_per_thread = (int)(num_bytes / (float) NUM_THREADS);

	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].start = start + i * num_bytes_per_thread;
		assignments[i].result = &result->macs[i];
		assignments[i].event_done = this->syncf->new_event(true);
		assignments[i].key = key;
		assignments[i].random_supply = random_supply;

		if (i < NUM_THREADS - 1) { 
			assignments[i].num_bytes = num_bytes_per_thread;
		} else { // Assign any leftovers to the final thread
			assignments[i].num_bytes = num_bytes - (i * num_bytes_per_thread);
		}
		//ZLC_TERSE(ze, "Dispatching a thread\n");
		this->tf->create_thread(parallel_mac, (void*)&assignments[i], STACK_SIZE);
	}

	// Wait for the results (serially, since we don't have a semaphor)
	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].event_done->wait();
		delete assignments[i].event_done;	// Clean up the memory
	}

	// Combine all of the results
	Mac* meta = key->mac(random_supply, (uint8_t*)&result->macs, 
	                     sizeof(Mac) * NUM_THREADS);
	memcpy(&result->metaMac, meta, sizeof(Mac));
	delete meta;
#else // NUM_THREADS == 1
	// TODO: Technically, this wastes sizeof(Mac) bytes, since we don't 
	// use the mac_list.mac field
	Mac* meta = key->mac(random_supply, start, num_bytes);
	memcpy(&result->metaMac, meta, sizeof(Mac));
	delete meta;
#endif // NUM_THREADS
}

bool ZFastFetch::distribute_mac_verify(uint8_t* start, uint32_t num_bytes, 
                                       mac_list* old_mac_list, MacKey* key) {
#if NUM_THREADS > 1
	mac_verify_assign assignments[NUM_THREADS];

	bool results[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {
		results[i] = false;
	}

	uint32_t num_bytes_per_thread = (int)(num_bytes / (float) NUM_THREADS);

	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].start = start + i * num_bytes_per_thread;
		assignments[i].result = &results[i];
		assignments[i].old_mac = &old_mac_list->macs[i];
		assignments[i].event_done = this->syncf->new_event(true);
		assignments[i].key = key;

		if (i < NUM_THREADS - 1) { 
			assignments[i].num_bytes = num_bytes_per_thread;
		} else { // Assign any leftovers to the final thread
			assignments[i].num_bytes = num_bytes - (i * num_bytes_per_thread);
		}

		ZLC_TERSE(ze, "Dispatching a thread with %d bytes\n",, assignments[i].num_bytes);
		this->tf->create_thread(parallel_mac_verify, (void*)&assignments[i], 
		                        STACK_SIZE);
	}

	// Wait for the results (serially, since we don't have a semaphor)
	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].event_done->wait();
		delete assignments[i].event_done;	// Clean up the memory
	}

	// Verify the individual results
	for (int i = 0; i < NUM_THREADS; i++) {
		if (!results[i]) {
			return false;
		}
	}

	// Check the meta mac 
	return key->verify(&old_mac_list->metaMac, (uint8_t*)&old_mac_list->macs, 
	                   sizeof(Mac) * NUM_THREADS);
#else // NUM_THREADS == 1
	return key->verify(&old_mac_list->metaMac, start, num_bytes);
#endif // NUM_THREADS
}

void ZFastFetch::distribute_hash(uint8_t* start, uint32_t num_bytes, hash_t* result) {
	hash_assign assignments[NUM_THREADS];
	hash_t results[NUM_THREADS];

	uint32_t num_bytes_per_thread = (int)(num_bytes / (float) NUM_THREADS);

	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].start = start + i * num_bytes_per_thread;
		assignments[i].result = &results[i];
		assignments[i].event_done = this->syncf->new_event(true);

		if (i < NUM_THREADS - 1) { 
			assignments[i].num_bytes = num_bytes_per_thread;
		} else { // Assign any leftovers to the final thread
			assignments[i].num_bytes = num_bytes - (i * num_bytes_per_thread);
		}
		//ZLC_TERSE(ze, "Dispatching a thread\n");
		this->tf->create_thread(parallel_hash, (void*)&assignments[i], STACK_SIZE);
	}

	// Wait for the results (serially, since we don't have a semaphor)
	for (int i = 0; i < NUM_THREADS; i++) {
		assignments[i].event_done->wait();
		delete assignments[i].event_done;	// Clean up the memory
	}

	// Combine all of the results
	zhash((uint8_t*)results, sizeof(hash_t)*NUM_THREADS, result);
}


ZFastFetch::ZFastFetch(
		ZLCEmit *ze,
		MallocFactory *mf,
		ThreadFactory *tf,
		SyncFactory *syncf,
		ZLookupClient *zlookup_client,
		UDPEndpoint *origin_zftp,
		SocketFactory *sockf,
		PerfMeasure *perf_measure,
		KeyDerivationKey* appKey, 
		RandomSupply* random_supply
		)
{
	this->ze = ze;
	this->mf = mf;
	this->tf = tf;
	this->syncf = syncf;
	this->zlookup_client = zlookup_client;
	this->origin_zftp = origin_zftp;
	this->sockf = sockf;
	this->perf_measure = perf_measure;
	this->random_supply = random_supply;

	// Derive a key to use for authenticating stored data
	char keyStr[] = "ZFTP MAC Key";
	this->mac_key = appKey->deriveMacKey((uint8_t*)keyStr, sizeof(keyStr));

	XIPAddr broadcast = get_ip_subnet_broadcast_address(ipv4);
	UDPEndpoint* keyval_endpoint = make_UDPEndpoint(
			this->mf,  &broadcast, KEYVAL_PORT);
	lite_assert(keyval_endpoint);

	AbstractSocket* keyval_sock = sockf->new_socket(sockf->get_inaddr_any(ipv4), true);
	lite_assert(keyval_sock);

	this->keyval_client = new KeyValClient(this->mf, keyval_sock, keyval_endpoint, ze);
	lite_assert(this->keyval_client);
}

ZFastFetch::~ZFastFetch()
{
	delete this->mac_key;
	delete this->keyval_client;
}

class ZFRFS_ZC : public ZeroCopyBuf
{
public:
	ZFRFS_ZC(ZFileReplyFromServer *reply) { this->reply = reply; }
	~ZFRFS_ZC() { delete reply; }
	virtual uint8_t *data() { return reply->get_payload(); }
	virtual uint32_t len() { return reply->get_payload_len(); }
	
private:
	ZFileReplyFromServer *reply;
};

// Implements DJBX33X: Treats key as a character string.
// At each stage, multiply hash by 33 and xor in a character
uint32_t insecure_hash(const char* key, const uint32_t key_len) {
	uint32_t hash = 5381;
	for (unsigned int i = 0; i < key_len; i++) {
		hash = ((hash << 5) + hash) ^ key[i];
	}
	return hash;
}


// Ask key-value server for the MAC for this file
// Expects an app and file specific string in key
// Returns true if mac contains a purported MAC value
bool ZFastFetch::lookup_mac(Key* key, mac_list* mac) {
	Value* val = this->keyval_client->lookup(key);

	if (val == NULL) {
		ZLC_COMPLAIN(ze, "FastFetch got NULL reply from KV server\n");
		return false;
	}

	if (val->len == 0) {
		// KV doesn't have this key
		mf_free(this->keyval_client->get_mf(), val);
		return false;
	} else if (val->len != sizeof(mac_list)) {
		mf_free(this->keyval_client->get_mf(), val);
		ZLC_COMPLAIN(ze, "FastFetch got bad MAC size %d from KV server\n",,val->len);
		return false;
	}
	memcpy(mac, val->bytes, sizeof(mac_list));

	mf_free(this->keyval_client->get_mf(), val);
	return true;
}

// Inserts mac value into the KeyValue server's store 
void ZFastFetch::insert_mac(Key* key, mac_list* mac) {
	KeyVal* kv = (KeyVal*)mf_malloc(this->mf, sizeof(KeyVal)+sizeof(mac_list));
	lite_assert(kv);
	kv->key = *key; 
	kv->val.len = sizeof(mac_list);
	memcpy(kv->val.bytes, mac, sizeof(mac_list));

	bool rc = this->keyval_client->insert(kv);
	mf_free(this->mf, kv);

	if (!rc) {
		ZLC_COMPLAIN(ze, "FastFetch failed to find KV server for insert\n");
	}
}

// vendor_name is name of app doing the fetching
// fetch_url is the desired file's URI
ZeroCopyBuf *ZFastFetch::fetch(const char * vendor_name, const char *fetch_url)
{
	hash_t requested_hash = zlookup_client->lookup_url(fetch_url);
	hash_t zh = get_zero_hash();

	if (cmp_hash(&requested_hash, &zh)==0)
	{
		return false;
	}

	ZFTPRequestPacket *zrp = (ZFTPRequestPacket *)
		mf_malloc(this->mf, sizeof(ZFTPRequestPacket));
	zrp->hash_len = z_htons(sizeof(hash_t));
	zrp->url_hint_len = z_htong(0, sizeof(zrp->url_hint_len));	// TODO may need to pass along
	zrp->file_hash = requested_hash;
	zrp->num_tree_locations = z_htong(0, sizeof(zrp->num_tree_locations));
		// don't need ANY merkle, since we trust ya! (or later,
		// when Bryan shows up here, since we have a full-file hash/MAC
		// to check)
	
	uint32_t padding_request = 0x1000 - 0x89+0xc;
		// TODO hard-coded padding offset. See notes 2012.07.05.
		// There are known factors in here (IPv6, UDP, ZFTP headers),
		// but also a host-dependent factor. So a correct implementation
		// will need to probe the host to figure out what that offset is.
		// We probably want that probe to be driven by zftp_zoog, so we
		// can avoid needing an extra probe round-trip in the fast path.

	zrp->padding_request = z_htong(padding_request, sizeof(zrp->padding_request));
	zrp->data_start = z_htong(0, sizeof(zrp->data_start));
	zrp->data_end = z_htong((uint32_t) -1, sizeof(zrp->data_end));

//	perf_measure->mark_time("ZFastFetch::fetch before send");
	AbstractSocket *sock = sockf->new_socket(
		sockf->get_inaddr_any(origin_zftp->ipaddr.version), false);
	bool rc = sock->sendto(origin_zftp, zrp, sizeof(ZFTPRequestPacket));
	lite_assert(rc);
	mf_free(mf, zrp);

	ZeroCopyBuf *zcb = sock->recvfrom(NULL);
//	perf_measure->mark_time("ZFastFetch::fetch after recv");
	delete sock;

	// Wrap decoded packet in a ZCB interface (which will reclaim the
	// packet on delete)
	ZFileReplyFromServer *reply = new ZFileReplyFromServer(zcb, ze);
	if (!reply->is_valid())
	{
		ZLC_COMPLAIN(ze, "FastFetch failed for %s\n",, fetch_url);
		delete reply;
		return NULL;
	}
	
	uint32_t alignment = (uint32_t) reply->get_payload();
	lite_assert((alignment & 0xfff)==0);	// can't use fast_mmap unless we get the zarfile aligned!
	// You might see this if you have a trivial zarfile (say, the stupid
	// default one that zftp_create_zarfile is fond of making if you
	// don't have paths yet). I think that causes the IP code to skip the
	// jumbo frame header, and change the alignment.

	ZFRFS_ZC *data_zcb = new ZFRFS_ZC(reply);

	// TODO plumb in manifest hash
	// compute and print zarfile hash. Here, like in the single-file fetch
	// case, we compute the hash, but we haven't plumbed through any manifest
	// information telling us what it *should* be, because that makes
	// development tedious -- we have to rebuild the loader every time we
	// rebuild the zarfile. But we want to at least be performing the
	// expensive crypto step so we're accounting correctly.
	// NB this is a full-file, byte-granularity-length hash, not the
	// Merkle-tree file hash (hash of metadata + root hash) at the top of
	// a zftp block tree.

	Key index = 0;
	if (fetch_url != NULL) {
		index ^= insecure_hash(fetch_url, lite_strlen(fetch_url));
	}
	if (vendor_name != NULL) {
		index ^= insecure_hash(vendor_name, lite_strlen(vendor_name));
	}


	// Ask key-value server for the previous MAC value
	mac_list old_mac_list;
	perf_measure->mark_time("mac_lookup_start");
	bool found_old_mac = lookup_mac(&index, &old_mac_list);
	perf_measure->mark_time("mac_lookup_end");

	if (!found_old_mac) {
		ZLC_TERSE(ze, "Didn't find an old MAC\n");
		// KV doesn't have a MAC for us, so check the hash
		perf_measure->mark_time("hash_start");
		hash_t actual_hash;

		// TODO: Compute this honestly, based on algorithm used to first compute it
		if (NUM_THREADS > 1) {
			distribute_hash(data_zcb->data(), data_zcb->len(), &actual_hash);
		} else {
			zhash(data_zcb->data(), data_zcb->len(), &actual_hash);
		}
		char buf[256];
		debug_hash_to_str(buf, &actual_hash);
		ZLC_TERSE(ze, "Zarfile hash is %s\n",, buf);
		// TODO: Compare with manifest hash here!
		perf_measure->mark_time("hash_end");

		// Compute and store a new MAC so we don't have to do the hash again
		perf_measure->mark_time("mac_start");
		mac_list new_mac_list;
		distribute_mac(data_zcb->data(), data_zcb->len(), 
		               &new_mac_list, random_supply, mac_key);
		insert_mac(&index, &new_mac_list);
		perf_measure->mark_time("mac_end");
	} else {
		ZLC_TERSE(ze, "Found an old MAC\n");
		
		// Verify the MAC we got back
		perf_measure->mark_time("mac_verify_start");
		bool valid_mac = distribute_mac_verify(data_zcb->data(), data_zcb->len(),
		                                       &old_mac_list, mac_key);
		perf_measure->mark_time("mac_verify_end");

		if (!valid_mac) {
			// Panic
			ZLC_COMPLAIN(ze, "Old MAC didn't match!\n");
		}
	}
	


	return data_zcb;
}
