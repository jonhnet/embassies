#include "LiteLib.h"
#include "SocketFactory_Skinny.h"
#include "sockaddr_util.h"
#include "zftp_util.h"
#include "simple_network.h"
#include "ZeroCopyBuf_Xnb.h"
#include "ZeroCopyView.h"

SkinnySocket::SkinnySocket(XaxSkinnyNetwork *xsn, UDPEndpoint *local, bool want_timeouts)
{
	this->xsn = xsn;
	this->want_timeouts = want_timeouts;

	// TODO should assert that local.ipaddr is DEFAULT (0.0.0.0 or 0::0) or
	// our ifconfig.

	uint16_t port_host_order = 0;
	if (local!=NULL)
	{
		port_host_order = Z_NTOHG(local->port);
	}

	xax_skinny_socket_init(&skinny_socket, xsn, port_host_order, local->ipaddr.version);
}

SkinnySocket::~SkinnySocket()
{
	xax_skinny_socket_free(&skinny_socket);
}

class SFS_ZC : public ZeroCopyBuf {
public:
	SFS_ZC(ZoogDispatchTable_v1 *xdt, uint32_t header_len, uint32_t payload_len);
	uint8_t *data() { return payload_view.data(); }
	uint32_t len() { return payload_view.len(); }

	uint8_t *header_data() { return zcbx.data(); }
	ZoogNetBuffer *peek_xnb() { return zcbx.peek_xnb(); }
	uint32_t get_payload_len() { return payload_len; }

protected:
	ZeroCopyBuf_Xnb zcbx;
	uint32_t payload_len;
	ZeroCopyView payload_view;
};

SFS_ZC::SFS_ZC(ZoogDispatchTable_v1 *xdt, uint32_t header_len, uint32_t payload_len)
	: zcbx(xdt, header_len+payload_len),
	  payload_len(payload_len),
	  payload_view(&zcbx, false, header_len, payload_len)
{ }

ZeroCopyBuf *SkinnySocket::zc_allocate(uint32_t payload_len)
{
	uint32_t packet_len = udp_packet_length(
		skinny_socket.local_ep.ipaddr.version, payload_len);
	SFS_ZC *zcb = new SFS_ZC(xsn->get_xdt(), packet_len - payload_len, payload_len);
	return zcb;
}

bool SkinnySocket::zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb)
{
	return _internal_send(remote, zcb, false);
}

bool SkinnySocket::_internal_send(UDPEndpoint *remote, ZeroCopyBuf *zcb, bool release)
{
	// NB we know that we only send() things that this
	// class also allocate_packet()ed, so it's okay to peek into the
	// private type of the ZCB, where the xnb is hiding.
	SFS_ZC *sfs_zc = (SFS_ZC *) zcb;

	// Scrawl the UDP header into the front of the packet.
	uint32_t payload_len = sfs_zc->get_payload_len();
	uint32_t packet_len = udp_packet_length(
		skinny_socket.local_ep.ipaddr.version, payload_len);
	create_udp_packet_buf(sfs_zc->header_data(), packet_len, skinny_socket.local_ep, *remote, payload_len);

	xax_skinny_socket_send(&skinny_socket, sfs_zc->peek_xnb(), release);
		// TODO How do we know when the send is complete and we have the
		// XNB back? Ugh.
	return true;
}

void SkinnySocket::zc_release(ZeroCopyBuf *zcb)
{
	delete zcb;
}

bool SkinnySocket::sendto(UDPEndpoint *remote, void *buf, uint32_t buf_len)
{
	ZeroCopyBuf *zcb = zc_allocate(buf_len);
	lite_memcpy(zcb->data(), buf, buf_len);
	return _internal_send(remote, zcb, true);
}

ZeroCopyBuf *SkinnySocket::recvfrom(UDPEndpoint *out_remote)
{
	UDPEndpoint remote;

	while (true)
	{
		ZeroCopyBuf *zcb = xax_skinny_socket_recv_packet(&skinny_socket, &remote);
		if (zcb==NULL)
		{
			// tick.
			// Skinny still converts its timeouts into these stupid
			// empty-buffer ticks.
			if (want_timeouts)
			{
				return NULL;
			}
			continue;
		}
		if (out_remote!=NULL)
		{
			*out_remote = remote;
		}
		return zcb;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

SocketFactory_Skinny::SocketFactory_Skinny(XaxSkinnyNetwork *xsn)
{
	this->xsn = xsn;
}

AbstractSocket *SocketFactory_Skinny::new_socket(UDPEndpoint *local, bool want_timeouts)
{
	SkinnySocket *ss = new SkinnySocket(xsn, local, want_timeouts);
	if (!ss->skinny_socket.valid)
	{
		delete ss;
		return NULL;
	}
	return ss;
}
