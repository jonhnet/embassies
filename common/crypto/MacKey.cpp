#include "MacKey.h"
#include "LiteLib.h"

MacKey::MacKey(RandomSupply* rand) : ZSymKey(rand) {
	vmac_set_key(this->bytes, &this->ctx);
}

MacKey::MacKey(uint8_t* buffer, uint32_t len) : ZSymKey(buffer, len) {
	vmac_set_key(this->bytes, &this->ctx);
}

Mac* MacKey::mac(RandomSupply* random_supply, uint8_t* buffer, uint32_t len) {
	// Pick a random nonce
	uint8_t nonce[16];
	random_supply->get_random_bytes(nonce, sizeof(nonce));

	return computeMac(buffer, len, nonce);
}

// VMAC expects to MAC a buffer that is 16-byte aligned and whose
// length is 0 mod 16, so we special-case beginning and end
// (see comments in vmac.h)
Mac* MacKey::computeMac(uint8_t* buffer, uint32_t len, uint8_t inc_nonce[16]) {
	Mac* ret = new Mac;

	// MAC the non-16-byte-aligned bytes at the beginning
	ret->prefixOverhang = (16 - ((uint32_t)buffer % 16)) % 16;
	if (ret->prefixOverhang > len)
	{
		ret->prefixOverhang = len;
	}

	if (ret->prefixOverhang != 0) {
		ALIGN(16) uint8_t prefix[VMAC_NHBYTES];
		lite_bzero(prefix, sizeof(prefix));

		// Copy over the overhang
		lite_memcpy(prefix, buffer, ret->prefixOverhang);

		vmac_update(prefix, sizeof(prefix), &this->ctx);
	}
	buffer += ret->prefixOverhang;
	len -= ret->prefixOverhang;
	
	// MAC the 16-byte-aligned VMAC_NHBYTES*n bytes in the middle
	ret->postfixOverhang = len % VMAC_NHBYTES;

	uint32_t body_len = len - ret->postfixOverhang;
	if (body_len < VMAC_NHBYTES) { 
		// Msg is too short, so we need padding, even for the body
		ALIGN(16) uint8_t body [VMAC_NHBYTES];
		lite_bzero(body, sizeof(body));
		lite_memcpy(body, buffer, body_len);

		vmac_update(body, sizeof(body), &this->ctx);
	} else {
		vmac_update(buffer, body_len, &this->ctx);
	}

	buffer += body_len;
	len = ret->postfixOverhang;

	// Copy over the overhang at the end
	ALIGN(16) uint8_t postfix[VMAC_NHBYTES];
	lite_bzero(postfix, sizeof(postfix));
	lite_memcpy(postfix, buffer, ret->postfixOverhang);

	// Make sure the nonce is aligned properly
	ALIGN(4) uint8_t nonce[16];
	lite_memcpy(nonce, inc_nonce, sizeof(nonce));
	nonce[0] &= 0xfe;	// Ensure first bit is 0 - TODO: Do this at nonce src

	// Finish the MAC off with the bytes at the end
	uint64_t mac_out = vmac(postfix, sizeof(postfix), nonce, NULL, &this->ctx);
	lite_memcpy(ret->mac, &mac_out, sizeof(mac_out));

	// Store the nonce we used
	lite_memcpy(ret->nonce, nonce, sizeof(nonce));

	return ret;
}

// TODO: Assumes buffer alignment is same as when we computed it!
bool MacKey::verify(Mac* alleged_mac, uint8_t* buffer, uint32_t len) {
	Mac* authentic_mac = computeMac(buffer, len, alleged_mac->nonce);

	return authentic_mac->prefixOverhang  == alleged_mac->prefixOverhang &&
	       authentic_mac->postfixOverhang == alleged_mac->postfixOverhang &&
				 lite_memcmp(authentic_mac->mac, alleged_mac->mac, MAC_SIZE) == 0;
}
