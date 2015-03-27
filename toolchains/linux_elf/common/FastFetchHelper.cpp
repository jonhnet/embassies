#include "cheesy_snprintf.h"
#include "ZLCEmit.h"
#include "zemit.h"
#include "zftp_protocol.h"
#include "format_ip.h"
#include "zftp_hash_lookup_protocol.h"
#include "FastFetchHelper.h"

FastFetchHelper::FastFetchHelper(
	XaxSkinnyNetwork *xax_skinny_network,
	SocketFactory *socket_factory,
	ZLCEmit* ze)
	: xax_skinny_network(xax_skinny_network),
	  socket_factory(socket_factory),
	  ze(ze)
{
}

void FastFetchHelper::_evil_get_server_addresses(
	UDPEndpoint *out_lookup_ep,
	UDPEndpoint *out_zftp_ep)
{
//	discovery_client_get_zftp_server_only(xpe->zdt, xpe->xax_skinny_network, ipv4, &out_zftp_ep->ipaddr);

	XIPifconfig *ipv4_ifconfig = xax_skinny_network->get_ifconfig(ipv4);
	out_zftp_ep->ipaddr = ipv4_ifconfig->gateway;
	
	out_zftp_ep->port = z_htons(ZFTP_PORT);
	*out_lookup_ep = *out_zftp_ep;
	out_lookup_ep->port = z_htons(ZFTP_HASH_LOOKUP_PORT);
}


bool FastFetchHelper::_find_fast_fetch_origin(UDPEndpoint *out_origin)
{
	// query ipv6 broadcast for anyone willing to answer ZFTP queries
	UDPEndpoint bcast_ep;
	bcast_ep.ipaddr = get_ip_subnet_broadcast_address(ipv6);
	bcast_ep.port = z_htons(ZFTP_PORT);

	AbstractSocket *test_socket = socket_factory->new_socket(
		socket_factory->get_inaddr_any(ipv6), NULL, true);
	if (test_socket==NULL)
	{
		ZLC_TERSE(ze, "_find_fast_fetch_origin: No ipv6 address available.\n");
		delete test_socket;
		return false;
	}

	{
		ZeroCopyBuf *zcb = test_socket->zc_allocate(sizeof(ZFTPRequestPacket));
		ZFTPRequestPacket *zrp = (ZFTPRequestPacket *) zcb->data();
		zrp->hash_len = z_htons(sizeof(hash_t));
		zrp->url_hint_len = z_htong(0, sizeof(zrp->url_hint_len));	// TODO may need to pass along
		zrp->file_hash = get_zero_hash();
		zrp->num_tree_locations = z_htong(0, sizeof(zrp->num_tree_locations));
		zrp->padding_request = z_htong(0, sizeof(zrp->padding_request));
		zrp->compression_context.state_id = z_htong(ZFTP_NO_COMPRESSION, sizeof(zrp->compression_context.state_id));
		zrp->data_start = z_htong(0, sizeof(zrp->data_start));
		zrp->data_end = z_htong(0, sizeof(zrp->data_end));

		bool rc = test_socket->zc_send(&bcast_ep, zcb, zcb->len());
		lite_assert(rc);

		test_socket->zc_release(zcb);
	}

	UDPEndpoint remote;
	ZeroCopyBuf *reply = test_socket->recvfrom(&remote);
	if (reply==NULL)
	{
		// timeout.
		ZLC_TERSE(ze, "_find_fast_fetch_origin: timeout polling server.\n");
		delete test_socket;
		return false;
	}

	char disp[100];
	if (reply->len() < sizeof(ZFTPReplyHeader))
	{
		cheesy_snprintf(disp, sizeof(disp), "short reply %d bytes", reply->len());
	}
	else
	{
		ZFTPReplyHeader *zrh = (ZFTPReplyHeader *) reply->data();
		cheesy_snprintf(disp, sizeof(disp), "reply code %x", Z_NTOHG(zrh->code));
	}

	char ipbuf[100];
	format_ip(ipbuf, sizeof(ipbuf), &remote.ipaddr);
	ZLC_TERSE(ze, "Got ipv6 zftp reply from %s; %s\n",, ipbuf, disp);
	*out_origin = remote;
	delete test_socket;
	return true;	// could be false when we learn to timeout
}


void FastFetchHelper::init_randomness(
	ZoogDispatchTable_v1* zdt,
	RandomSupply** out_random_supply,
	KeyDerivationKey** out_appKey)
{
	zdt->zoog_get_random(sizeof(seed), seed);
	*out_random_supply = new RandomSupply(seed);

	uint8_t app_secret[SYM_KEY_BITS/8];
	zdt->zoog_get_app_secret(sizeof(app_secret), app_secret);
	*out_appKey = new KeyDerivationKey(app_secret, sizeof(app_secret));
}
