#include "SocketFactory.h"

bool AbstractSocket::sendto(UDPEndpoint *remote, void *buf, uint32_t buf_len)
{
	ZeroCopyBuf *zcb = zc_allocate(buf_len);
	lite_memcpy(zcb->data(), buf, buf_len);
	bool result = zc_send(remote, zcb, zcb->len());
	zc_release(zcb);
	return result;
}

bool SocketFactory::any_init = false;
UDPEndpoint SocketFactory::any[2];

UDPEndpoint *SocketFactory::get_inaddr_any(XIPVer ipver)
{
	if (!any_init)
	{
		lite_memset(&any[0], 0, sizeof(any[0]));
		any[0].ipaddr.version = ipv4;
		lite_memset(&any[1], 0, sizeof(any[1]));
		any[1].ipaddr.version = ipv6;
	}

	switch (ipver)
	{
	case ipv4:
		return &any[0];
	case ipv6:
		return &any[1];
	default:
		lite_assert(false);
		return NULL;
	}
}

