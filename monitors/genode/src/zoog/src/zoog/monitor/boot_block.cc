#include "boot_block.h"

ZoogMonitor::BootBlock::BootBlock(SignedBinary* sb, uint32_t capacity)
{
	_cert_len = Z_NTOHG(sb->cert_len);
	_binary_len = Z_NTOHG(sb->binary_len);
	uint32_t sb_len = sizeof(*sb) + _cert_len + _binary_len;
	lite_assert(sb_len == capacity);

	bool rc;
	rc = Genode::env()->heap()->alloc(sb_len, &this->_storage);
	lite_assert(rc);
	Genode::memcpy(this->_storage, sb, sb_len);
	this->_len = sb_len;
}

ZoogMonitor::BootBlock::~BootBlock()
{
	Genode::env()->heap()->free(_storage, _len);
}

ZoogMonitor::BootBlock* ZoogMonitor::BootBlock::get_initial_boot_block()
{
	return new BootBlock(
		(SignedBinary*) &_binary_zoog_boot_signed_start,
		&_binary_zoog_boot_signed_end - &_binary_zoog_boot_signed_start);
}

ZCert* ZoogMonitor::BootBlock::copy_cert()
{
	SignedBinary* sb = (SignedBinary*) _storage;
	uint8_t *cert_start = (uint8_t*) (&sb[1]);
	ZCert *cert = new ZCert(cert_start, _cert_len);
	return cert;
}
