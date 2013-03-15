#include "ByteStream.h"
#include "CryptoException.h"
#include "zoog_network_order.h"

// Assumes memory region [buffer, buffer+size) is valid
// and remains valid for the lifetime of the ByteStream
// Does not make a local copy!
ByteStream::ByteStream(uint8_t* buffer, uint32_t size) {
	if (!buffer || size == 0) { Throw(CryptoException::BAD_INPUT, "Tried to create an empty ByteStream"); }

	this->buffer = buffer;
	this->size = size;
	this->pos = 0;
}

void ByteStream::assertBytesRemaining(uint32_t numBytes) {
	if (this->pos + numBytes < this->pos ||
		this->pos + numBytes < numBytes) {
		Throw(CryptoException::INTEGER_OVERFLOW, "Overflow while checking on a ByteStream!");
	}

	if (this->pos + numBytes > this->size) {
		Throw(CryptoException::BUFFER_OVERFLOW, "Tried to read or write past the end of a ByteStream!");
	}
}

void ByteStream::assertInBounds(uint32_t index) {
	if (index >= this->size) {
		Throw(CryptoException::BUFFER_OVERFLOW, "Tried to read or write past the end of a ByteStream!");
	}
}

ByteStream& operator<<(ByteStream& out, uint8_t value) {
	out.assertBytesRemaining(sizeof(uint8_t));
	out.buffer[out.pos] = value;
	out.pos += sizeof(uint8_t);
	return out;
}

ByteStream& operator<<(ByteStream& out, uint16_t value) {
	out.assertBytesRemaining(sizeof(uint16_t));
	*((uint16_t*)(&out.buffer[out.pos])) = z_htons(value);
	out.pos += sizeof(uint16_t);
	return out;
}

ByteStream& operator<<(ByteStream& out, uint32_t value) {
	out.assertBytesRemaining(sizeof(uint32_t));
	*((uint32_t*)(&out.buffer[out.pos])) = z_htons(value);
	out.pos += sizeof(uint32_t);
	return out;
}

ByteStream& operator>>(ByteStream& in, uint8_t &value) {
	in.assertBytesRemaining(sizeof(uint8_t));
	value = in.buffer[in.pos];
	in.pos += sizeof(uint8_t);
	return in;
}

ByteStream& operator>>(ByteStream& in, uint16_t &value) {
	in.assertBytesRemaining(sizeof(uint16_t));
	value = z_ntohs(*((uint16_t*)&in.buffer[in.pos]));
	in.pos += sizeof(uint16_t);
	return in;
}

ByteStream& operator>>(ByteStream& in, uint32_t &value) {
	in.assertBytesRemaining(sizeof(uint32_t));
	value = z_ntohs(*((uint32_t*)&in.buffer[in.pos]));
	in.pos += sizeof(uint32_t);
	return in;
}

uint8_t& ByteStream::operator[] (const int index) {
	this->assertInBounds(index);
	return this->buffer[index];
}
