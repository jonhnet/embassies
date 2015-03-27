#pragma once

#include "xax_skinny_network.h"
#include "SocketFactory.h"
#include "ZLCEmit.h"
#include "KeyDerivationKey.h"

class FastFetchHelper
{
private:
	XaxSkinnyNetwork* xax_skinny_network;
	SocketFactory* socket_factory;
	ZLCEmit* ze;

	uint8_t seed[RandomSupply::SEED_SIZE];
	uint8_t app_secret[SYM_KEY_BITS/8];

public:
	FastFetchHelper(
		XaxSkinnyNetwork *xax_skinny_network,
		SocketFactory *socket_factory,
		ZLCEmit* ze);

	void _evil_get_server_addresses(
		UDPEndpoint *out_lookup_ep,
		UDPEndpoint *out_zftp_ep);

	bool _find_fast_fetch_origin(UDPEndpoint *out_origin);

	void init_randomness(
		ZoogDispatchTable_v1* zdt,
		RandomSupply** out_random_supply,
		KeyDerivationKey** out_appKey);
};
