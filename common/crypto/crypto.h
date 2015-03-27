/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#ifndef _CRYPTO_H
#define _CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif
#include "sha1.h"
#include "sha2.h"
#include "vmac.h"
#include "rijndael.h"

#include "types.h"

/*************************************************************
 * Note: Make sure your compiler has enabled SSE2 support!   *
 *       In VS: Project Properties->C/C++->Code Generation   *
 *              Enable Enhanced Instruction Set (/arch:SSE2) *
 *************************************************************/

// Choose your hash function
#define USE_SHA1 1
#define USE_SHA256 0
#define USE_SHA512 0

// Choose your block cipher (only one choice at the moment)
#define USE_AES

#define MAX_AES_ROUNDS 14

// Defined in bits
#ifdef USE_AES
#define BLOCK_CIPHER_SIZE 128
#endif // USE_AES

// Default key sizes defined in bits
#define PUB_KEY_SIZE 2048
#ifdef USE_AES
#define SYM_KEY_BITS 128
#define SYM_KEY_BYTES (SYM_KEY_BITS / 8)
#define ROUND_KEY_SIZE (4*(MAX_AES_ROUNDS+1))
#endif // USE_AES

// Size of hash output in bytes
#if USE_SHA1
#define HASH_SIZE SPH_SIZE_sha1 / 8
#elif  USE_SHA256
#define HASH_SIZE SPH_SIZE_sha256 / 8
#elif  USE_SHA512
#define HASH_SIZE SPH_SIZE_sha512 / 8
#endif 

typedef struct {
	uint8_t bytes[HASH_SIZE];
} hash_t;

/* 
 * Inputs: Buffer to be hashed and its size in bytes
 * Outputs: Hash result in the buffer provided.  
 *          Assumed to be correctly sized at HASH_SIZE.
 */
void zhash(const uint8_t* input, uint32_t size, hash_t* hash);

//#define MAC_CTX vmac_ctx_t
#define MAC_SIZE VMAC_TAG_LEN / 8
//#define MAC_NONCE_SIZE VMAC_NHBYTES / 8
#define MAC_KEY_SIZE VMAC_KEY_LEN / 8

///* 
// * Inputs: Buffer to be MAC'ed and its size in bytes,
// *         a unique nonce value of size MAC_NONCE_SIZE,
// *		   a buffer of key material, assumed to be MAC_KEY_SIZE
// * Outputs: MAC tag result in the buffer provided.  
// *          Assumed to be correctly sized at MAC_SIZE
// */
//void MAC(uint8_t* input, uint32_t size, uint8_t* nonce, uint8_t* key, uint8_t* tag);
//
//// As an optimization, you can prepare the MAC key once, and then compute many MACs
//
///* 
// * Inputs: A buffer of key material, assumed to be MAC_KEY_SIZE
// *		   A pointer to a MAC context: MAC_CTX
// * Outputs: None.
// */
//void MAC_set_key(uint8_t* key, MAC_CTX* ctx);
//
///* 
// * Inputs: Buffer to be MAC'ed and its size in bytes,
// *         a unique nonce value of size MAC_NONCE_SIZE,
// *		   a MAC_CTX that has been initialized with MAC_set_key
// * Outputs: MAC tag result in the buffer provided.  
// *          Assumed to be correctly sized at MAC_SIZE
// */
//void MAC_compute(uint8_t* input, uint32_t size, uint8_t* nonce, MAC_CTX* ctx, uint8_t* tag);
//

typedef struct {
	uint8_t bytes[BLOCK_CIPHER_SIZE/8];
} app_secret_t;


#ifdef __cplusplus
}
#endif 

#endif //_CRYPTO_H
