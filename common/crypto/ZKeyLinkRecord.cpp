//#include <assert.h>
//#ifndef _WIN32
//#include <string.h>
//#endif // !_WIN32

#include "LiteLib.h"

#include "ZKeyLinkRecord.h"
#include "ZCert.h"
#include "crypto.h"
#include "CryptoException.h"


ZKeyLinkRecord::ZKeyLinkRecord(uint8_t* txtRecord, uint32_t size) {
	if(!txtRecord) { Throw(CryptoException::BAD_INPUT, "Null record"); }
	uint8_t* txtRecordOrig = txtRecord;

	// Parse the generic DNS record info
	txtRecord = parse(txtRecord, size);

	// Now get the two keys this link connects
	// (accounting for the fact that we've used up some of the bytes)
	this->keySpeaker = new ZPubKeyRecord(txtRecord, size - (txtRecord - txtRecordOrig));
	if (!this->keySpeaker) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate keySpeaksfor"); }
	txtRecord += this->keySpeaker->size();

	this->keySpokenFor = new ZPubKeyRecord(txtRecord, size - (txtRecord - txtRecordOrig));
	if (!this->keySpokenFor) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate keySpokenFor"); }
	txtRecord += this->keySpokenFor->size();		
}

// Write the record into the buffer and return number of bytes written
uint32_t ZKeyLinkRecord::serialize(uint8_t* buffer) {
	if(!buffer) { Throw(CryptoException::BAD_INPUT, "Null buffer"); }
	if(!this->keySpeaker || !this->keySpokenFor) { Throw(CryptoException::BAD_INPUT, "Incomplete KeyLinkRecord"); }

	// Let the parent handle the generic data
	uint32_t len = ((ZRecord*)this)->ZRecord::serialize(buffer);
	buffer += len;

	// Now write the key link specific info
	len = this->keySpeaker->serialize(buffer);
	buffer += len;

	len = this->keySpokenFor->serialize(buffer);
	buffer += len;
	
	return len;
}

ZKeyLinkRecord::ZKeyLinkRecord(ZPubKey* keySpeaker, ZPubKey* keySpokenFor) 
	: ZRecord(NULL, ZOOG_RECORD_TYPE_KEYLINK ) {
	if (keySpeaker == NULL || keySpokenFor == NULL) { Throw(CryptoException::BAD_INPUT, "Tried to create KeyLink with 1 or more NULL keys") ;}

	this->keySpeaker = new ZPubKeyRecord(keySpeaker->getRecord());
	this->keySpokenFor = new ZPubKeyRecord(keySpokenFor->getRecord());

	this->addDataLength(this->keySpeaker->size() + this->keySpokenFor->size());  
}

bool ZKeyLinkRecord::isEqual(const ZRecord* z) const {
	if (z == NULL) {
		return false;
	}

	if (z->getType() == ZRecord::ZOOG_RECORD_TYPE_KEYLINK) {
		ZKeyLinkRecord* r = (ZKeyLinkRecord*)z;

		if ((this->keySpeaker == NULL && r->keySpeaker != NULL) ||
			(this->keySpeaker != NULL && r->keySpeaker == NULL) ||
			(this->keySpokenFor == NULL && r->keySpokenFor != NULL) ||
			(this->keySpokenFor == NULL && r->keySpokenFor != NULL)) {
			return false;
		}

		bool keySpeakerEq = true;
		if (this->keySpeaker != NULL) {
			keySpeakerEq = this->keySpeaker->isEqual(r->keySpeaker);
		} // Else, they're both NULL

		bool keySpokenForEq = true;
		if (this->keySpokenFor != NULL) {
			keySpokenForEq = this->keySpokenFor->isEqual(r->keySpokenFor);
		} // Else, they're both NULL

		return keySpeakerEq && keySpokenForEq;			
	}

	return false;
}

//bool operator==(ZKeyLinkRecord &r1, ZKeyLinkRecord &r2) {
//	if (r1.equal(&r2) && r1.flags == r2.flags && r1.digestType == r2.digestType) {
//		for (uint32_t i = 0; i < (uint32_t) (r1.dataLength - 4); i++) {
//			if (r1.digest[i] != r2.digest[i]) {
//				return false;
//			}
//		}
//		return true;
//	} 
//	return false;		
//}
//
//bool operator!=(ZKeyLinkRecord &r1, ZKeyLinkRecord &r2) {
//	return !(r1 == r2);
//}

ZKeyLinkRecord::~ZKeyLinkRecord() {
	delete this->keySpeaker;
	delete this->keySpokenFor;
}

