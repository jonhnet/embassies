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
#pragma once

/*  
 * Our certificates are designed to interoperate with DNSSEC,
 * as defined in:
 *	 RFC 4033: http://tools.ietf.org/html/rfc4033
 *	 RFC 4034: http://tools.ietf.org/html/rfc4034
 *	 RFC 4035: http://tools.ietf.org/html/rfc4035
 *
 * RSA/SHA-2 implementation based on proposed RFC 5702 http://tools.ietf.org/html/rfc5702
 */


#include "ZBinaryRecord.h"
#include "ZPubKeyRecord.h"
#include "ZSigRecord.h"
#include "ZPubKey.h"

// Forward declare these, since they're mutually recursive
class ZPubKey;
class ZPrivKey;
class ZPubKeyRecord;

// Types of digest supported
#define ZOOG_DIGEST_SHA_1	1	/* Consistent with RFC 4034, A.2 */
#define ZOOG_DIGEST_SHA_256	2	/* Introduced by Zoog */
#define ZOOG_DIGEST_SHA_512 3	/* Introduced by Zoog */

#define ZOOG_DIGEST_DEFAULT ZOOG_DIGEST_SHA_512

// Code using this module must provide some source of trustworthy time in Unix seconds,
// i.e.,  number of seconds elapsed since 1 January 1970 00:00:00 UTC, ignoring leap seconds
extern uint32_t getCurrentTime();

class ZCert {
public:
	// Try to approximate the size of the largest cert we'll produce
	// The largest object we're likely to sign is another key, so plan on at least 2 key records
	// 3 + 2048/8 is the approximate size of a public key
	static const uint32_t MAX_CERT_SIZE = 3000; /* 3*DomainName::MAX_DNS_NAME_LEN + 
																sizeof(ZRecord) + 3 + 2048/8 +
																sizeof(ZSigRecord) + ZKey::SIGNATURE_SIZE +
																sizeof(ZPubKeyRecord) +  3 + 2048/8 + 
																1000;  // Add an extra 1000 for safety; */

	// Create a cert based on one that was previously serialized
	ZCert(uint8_t* serializedCert, uint32_t size);

  // DNSSEC records of what has been signed, and the acommpanying signature, and the key used for signing
  ZCert(uint8_t* record, uint32_t rsize, uint8_t* sigRecord, uint32_t ssize, uint8_t* signingKeyRecord, uint32_t ksize);

  // Initialize a certificate describing a binding from a key to a name
  // Semantics: The key that signs this cert says <<key speaksfor the name in label>>
  ZCert(uint32_t inception, uint32_t expires, const DomainName* label, ZPubKey* key);

  // Initialize a certificate for a binary
  // Semantics: The key that signs this cert says <<binary speaksfor me>> 
  ZCert(uint32_t inception, uint32_t expires, const uint8_t* binary, uint32_t binaryLen);

  // Initialize a certificate describing a binding from keyA to keyB
  // Semantics: The key that signs this cert says <<keySpeaker speaksfor keySpokenFor>>
  ZCert(uint32_t inception, uint32_t expires, ZPubKey* keySpeaker, ZPubKey* keySpokenFor);

  ~ZCert();

  // Retrieve information about the binary signed by this certificate 
  // Returns null if this is not a certificate for a binary
  ZBinaryRecord* getEndorsedBinary();

  // Retrieve the key signed by this certificate (not the signing key)
  // Returns null if this is not a certificate for a key
  ZPubKey* getEndorsedKey();

	// Retrieve the public key corresponding to the private key that signed this certificate 
	// Returns NULL if the certificate is unsigned
	ZPubKey* getEndorsingKey();

	// If this is a certificate for a key link, 
	// i.e., a cert that says KeyA speaksfor KeyB, 
	// then getSpeakerKey returns KeyA
	// and getSpokenKey returns KeyB
	// Otherwise, returns NULL
	ZPubKey* getSpeakerKey();
	ZPubKey* getSpokenKey();

  // Number of bytes needed to serialize this cert
  uint32_t size() const;

  // Write the entire certificate out
  void serialize(uint8_t* buffer);

  // Read it back in.  Assumes memory range [bytes, bytes+size) is valid
  ZCert* deserialize(uint8_t* bytes, uint32_t size);

  // Verify the validity of this certificate using the key provided
  bool verify(ZPubKey* key);

  // Verify the validity of this certificate using the cert chain provided, with rootkey as a trust anchor
  // At each step, the label on the signed object must have the parent's label chain as a suffix;
  // i.e., .com can sign example.com, which can sign www.example.com, but neither can sign harvard.edu
  // Chain should be arranged in order of increasing specificity;
  // i.e., trustAnchor signs key1 signs key2 signs key3 ... signs this cert
  //bool verifyTrustChain(ZCert** certs, uint32_t numCerts, ZPubKey* trustAnchor);

  // Sign this certificate using the key provided
  void sign(ZPrivKey* key);

  friend bool operator==(ZCert &c1, ZCert &c2);
  friend bool operator!=(ZCert &c1, ZCert &c2);

private:
  ZRecord* object;    // The thing that has been signed
  ZSigRecord* sig;    // Store info about the signature
  ZPubKeyRecord* signingKey; // Store info about the key that signed me
};

/* In DNSSEC, you can use one key (known by its DNSKEY RR) 
   to create a RRSIG RR on another key (known by its DNSKEY RR).
   This is how you sign your own keys (e.g., a KSK signs a ZSK).
   You can also sign your children's keys.  To do so, you use your
   key (known by its DNSKEY RR) to create a signature (an RRSIG RR)
   on a delegation (known by its DS RR)
   which summarizes the child's key (known by its DNSKEY RR).
   Finally, in Zoog, we would like to support signatures on binaries.
   This could be done by describing the binary in a DNS TXT record,
   and then creating a RRSIG RR on that record using your key (known by its DNSKEY RR) */

