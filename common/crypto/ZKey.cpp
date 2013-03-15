//#include <assert.h>
#ifndef _WIN32
//#include <string.h>
#endif // !_WIN32

#include "LiteLib.h"
#include "ZKey.h"
#include "CryptoException.h"

//ZKey::ZKey() {
//	this->rsa_ctx = new rsa_context;
//	rsa_init(this->rsa_ctx, RSA_PKCS_V15, RSA_SHA512);
//}

ZKey::ZKey(rsa_context* rsa_ctx, const DomainName* owner) {
	this->rsa_ctx = rsa_ctx;
	this->owner = new DomainName(owner);
	lite_assert(this->owner);
}


// Create a key from one we previously serialized
// Format: Owner, RSA params, RSA vals = length || val
ZKey::ZKey(uint8_t* buffer, uint32_t size) {
	this->parseKey(buffer, size);
}

#define WRITE_MPI(buffer, mpi) \
{\
	uint32_t size = mpi_size(mpi);	\
	WRITE_INT((buffer), size); \
	mpi_write_binary((mpi), buffer, size); \
	(buffer) += size; \
}

#define PARSE_MPI(buffer, mpi, bufferSize) \
{ \
	if (bufferSize < sizeof(uint32_t)) { Throw(CryptoException::BAD_INPUT, "Key too short"); } \
	uint32_t t_size;	\
	PARSE_INT((buffer), t_size); \
	bufferSize -= sizeof(uint32_t); \
	if (t_size > bufferSize) { Throw(CryptoException::BAD_INPUT, "Key too short"); } \
	mpi_read_binary((mpi), buffer, t_size); \
	(buffer) += t_size; \
}


// Format: TotalLen, Owner, RSA params, RSA vals = length || val
uint32_t ZKey::serializeKey(uint8_t* buffer) {
	this->owner->serialize(buffer);
	buffer += this->owner->size();

	WRITE_INT(buffer, this->rsa_ctx->ver);
	WRITE_INT(buffer, this->rsa_ctx->len);
	WRITE_INT(buffer, this->rsa_ctx->padding);
	WRITE_INT(buffer, this->rsa_ctx->hash_id);

	WRITE_MPI(buffer, &this->rsa_ctx->N);
	WRITE_MPI(buffer, &this->rsa_ctx->E);

	WRITE_MPI(buffer, &this->rsa_ctx->D);
	WRITE_MPI(buffer, &this->rsa_ctx->P);
	WRITE_MPI(buffer, &this->rsa_ctx->Q);
	WRITE_MPI(buffer, &this->rsa_ctx->DP);
	WRITE_MPI(buffer, &this->rsa_ctx->DQ);
	WRITE_MPI(buffer, &this->rsa_ctx->QP);

	//WRITE_MPI(buffer, &this->rsa_ctx->RN);
	//WRITE_MPI(buffer, &this->rsa_ctx->RP);
	//WRITE_MPI(buffer, &this->rsa_ctx->RQ);
	
	return this->keySize();
}

void ZKey::parseKey(uint8_t* buffer, uint32_t size) {
	this->owner = new DomainName(buffer, size);
	if(!this->owner) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate owner"); }
	buffer += this->owner->size();

	this->rsa_ctx = new rsa_context;
	if(!this->rsa_ctx) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate rsa_ctx"); }
	lite_memset(this->rsa_ctx, 0, sizeof( rsa_context ) );

	if ((uint32_t)(this->owner->size() + 4*sizeof(uint32_t)) > size) {
		Throw(CryptoException::BAD_INPUT, "Key too short");
	}

	PARSE_INT(buffer, this->rsa_ctx->ver);
	PARSE_INT(buffer, this->rsa_ctx->len);
	PARSE_INT(buffer, this->rsa_ctx->padding);
	PARSE_INT(buffer, this->rsa_ctx->hash_id);

	// Calculate how many bytes remain for the MPI data
	size = size - this->owner->size() - 4*sizeof(uint32_t);

	// TODO: Need to make sure we read these safely
	PARSE_MPI(buffer, &this->rsa_ctx->N, size);
	PARSE_MPI(buffer, &this->rsa_ctx->E, size);

	PARSE_MPI(buffer, &this->rsa_ctx->D, size);
	PARSE_MPI(buffer, &this->rsa_ctx->P, size);
	PARSE_MPI(buffer, &this->rsa_ctx->Q, size);
	PARSE_MPI(buffer, &this->rsa_ctx->DP, size);
	PARSE_MPI(buffer, &this->rsa_ctx->DQ, size);
	PARSE_MPI(buffer, &this->rsa_ctx->QP, size);

	//PARSE_MPI(buffer, &this->rsa_ctx->RN, size);
	//PARSE_MPI(buffer, &this->rsa_ctx->RP, size);
	//PARSE_MPI(buffer, &this->rsa_ctx->RQ, size);	
}


// Number of bytes needed to serialize the key
uint32_t ZKey::keySize() {
	uint32_t size = 0;

	size += this->owner->size();

	size += 4 * 4; // 4 int parameters
	size += 4 * 8; // 8 int sizes of the MPIs

	size += mpi_size(&this->rsa_ctx->N);
	size += mpi_size(&this->rsa_ctx->E);

	size += mpi_size(&this->rsa_ctx->D);
	size += mpi_size(&this->rsa_ctx->P);
	size += mpi_size(&this->rsa_ctx->Q);
	size += mpi_size(&this->rsa_ctx->DP);
	size += mpi_size(&this->rsa_ctx->DQ);
	size += mpi_size(&this->rsa_ctx->QP);

	//size += mpi_size(&this->rsa_ctx->RN);
	//size += mpi_size(&this->rsa_ctx->RP);
	//size += mpi_size(&this->rsa_ctx->RQ);

	return size;
}
