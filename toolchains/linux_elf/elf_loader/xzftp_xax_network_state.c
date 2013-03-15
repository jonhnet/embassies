#include "xzftp_xax_network_state.h"
#include "zclock.h"
#include "xax_network_utils.h"
#include "discovery_client.h"

void
zxax_virtual_destructor(ZFTPNetworkState *netstate)
{
	ZFTPNetXax *zxax = (ZFTPNetXax *)netstate;
	xax_skinny_socket_free(&zxax->skinny_socket);
	mf_free(zxax->mf, zxax);
}

void
zxax_get_endpoints(ZFTPNetworkState *netstate, UDPEndpoint *out_local_ep, UDPEndpoint *out_remote_ep)
{
	ZFTPNetXax *zxax = (ZFTPNetXax *)netstate;
	memcpy(out_local_ep, &zxax->skinny_socket.local_ep, sizeof(UDPEndpoint));
	memcpy(out_remote_ep, &zxax->remote_ep, sizeof(UDPEndpoint));
}

static void
zxax_send_packet(ZFTPNetworkState *netstate, IP4Header *ipp)
{
	ZFTPNetXax *zxax = (ZFTPNetXax *)netstate;
	xax_skinny_socket_send_packet(&zxax->skinny_socket, ipp);
}

static int
zxax_recv_packet(ZFTPNetworkState *netstate, uint8_t *buf, int data_len, UDPEndpoint *sender_ep)
{
	ZFTPNetXax *zxax = (ZFTPNetXax *)netstate;
	return xax_skinny_socket_recv_packet(
		&zxax->skinny_socket, buf, data_len, sender_ep);
}

ZFTPNetworkState *xzftp_xax_network_state_new(XaxPosixEmulation *xpe)
{
	MallocFactory *mf = xpe->mf;

	ZFTPNetXax *zxax = mf_malloc(mf, sizeof(ZFTPNetXax));

	zxax->mf = mf;
	zxax->methods.zm_virtual_destructor = zxax_virtual_destructor;
	zxax->methods.zm_get_endpoints = zxax_get_endpoints;
	zxax->methods.zm_send_packet = zxax_send_packet;
	zxax->methods.zm_recv_packet = zxax_recv_packet;

	discovery_client_get_zftp_server_only(xpe->xdt, &xpe->xax_skinny_network, &zxax->remote_ep.ip);

	zxax->remote_ep.port = z_htons(ZFTP_PORT);

	xax_skinny_socket_init(&zxax->skinny_socket, &xpe->xax_skinny_network);

	return (ZFTPNetworkState*) zxax;
}
