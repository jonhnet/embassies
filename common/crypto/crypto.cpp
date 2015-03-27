#include "crypto.h"
#include "sha-openssl.h"

#ifdef USE_SHA1

#if 0
void zhash(const uint8_t* input, uint32_t size, hash_t* hash) {
	sph_sha1_context sha_ctx; 
	sph_sha1_init(&sha_ctx); 
	sph_sha1(&sha_ctx, input, size); 
	sph_sha1_close(&sha_ctx, (uint8_t*)hash); 
}
#else
// Use the optimized OpenSSL version instead
void zhash(const uint8_t* input, uint32_t size, hash_t* hash) {
	SHA_CTX sha_ctx;
	sha1_init(&sha_ctx); 
	sha1_update(&sha_ctx, input, size); 
	sha1_final(&sha_ctx, (uint8_t*)hash); 
}
#endif

#elif USE_SHA256
void zhash(const uint8_t* input, uint32_t size, hash_t* hash) {
	sph_sha256_context sha_ctx; 
	sph_sha256_init(&sha_ctx); 
	sph_sha256(&sha_ctx, input, size); 
	sph_sha256_close(&sha_ctx, (uint8_t*)hash); 
}
#elif USE_SHA512
void zhash(const uint8_t* input, uint32_t size, hash_t* hash) {
	sph_sha512_context sha_ctx; 
	sph_sha512_init(&sha_ctx); 
	sph_sha512(&sha_ctx, input, size); 
	sph_sha512_close(&sha_ctx, (uint8_t*)hash); 
}
#endif // USE_SHA256

//void MAC(uint8_t* input, uint32_t size, uint8_t* nonce, uint8_t* key, uint8_t* tag) {
//	vmac_ctx_t mac_ctx;
//
//	MAC_set_key(key, &mac_ctx);
//	MAC_compute(input, size, nonce, &mac_ctx, tag);
//}
//
//void MAC_set_key(uint8_t* key, MAC_CTX* ctx) {
//	vmac_set_key(key, ctx);
//}
//
//void MAC_compute(uint8_t* input, uint32_t size, uint8_t* nonce, MAC_CTX* ctx, uint8_t* tag) {
//	uint64_t tag1, tag2;
//
//	tag1 = vmac(input, size, nonce, &tag2, ctx);
//
//	if (VMAC_TAG_LEN == 64) {
//		((uint64_t*)tag)[0] = tag1;
//	}
//
//	if (VMAC_TAG_LEN == 128) {
//		((uint64_t*)tag)[0] = tag2;
//		((uint64_t*)tag)[1] = tag1;
//	}
//}
//

//int load_pub_priv_key(priv_key_t* key) {
//	rsa_init(key, RSA_PKCS_V15, RSA_SHA1);
//  return rsa_gen_key(key, PUB_KEY_SIZE, 65537, NULL);
//}
//
//int sign(priv_key_t* key, uint8_t* message, uint32_t msg_len, uint8_t* sig) {
//  uint8_t hash_result[HASH_SIZE];
//
//  hash(message, msg_len, hash_result);
//  return rsa_pkcs1_sign(key, RSA_PRIVATE, RSA_SHA1, HASH_SIZE, hash_result, sig);
//}
//
//int verify(pub_key_t* key, uint8_t* message, uint32_t msg_len, uint8_t* sig) {
//  uint8_t hash_result[HASH_SIZE];
//
//  hash(message, msg_len, hash_result);
//  return rsa_pkcs1_verify(key, RSA_PUBLIC, RSA_SHA1, HASH_SIZE, hash_result, sig);
//}

