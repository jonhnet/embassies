#include "SocketFactory_Counter.h"

CounterSocket::CounterSocket(AbstractSocket* us, SocketFactory_Counter *sfc)
	: us(us),
	  sfc(sfc)
{}

CounterSocket::~CounterSocket()
{
	delete us;
}
	
ZeroCopyBuf *CounterSocket::zc_allocate(uint32_t payload_len)
{
	return us->zc_allocate(payload_len);
}

bool CounterSocket::zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb)
{
	sfc->_count(zcb->len());
	return us->zc_send(remote, zcb);
}
void CounterSocket::zc_release(ZeroCopyBuf *zcb)
{
	us->zc_release(zcb);
}

ZeroCopyBuf *CounterSocket::recvfrom(UDPEndpoint *out_remote)
{
	return us->recvfrom(out_remote);
}

//////////////////////////////////////////////////////////////////////////////

SocketFactory_Counter::SocketFactory_Counter(SocketFactory* usf)
	: usf(usf),
	  sent_bytes(0),
	  received_bytes(0)
{}

AbstractSocket *SocketFactory_Counter::new_socket(UDPEndpoint *local, bool want_timeouts)
{
	return new CounterSocket(usf->new_socket(local, want_timeouts), this);
}

void SocketFactory_Counter::_count(int _sent_bytes)
{
	int old_mb = sent_bytes>>20;
	this->sent_bytes += _sent_bytes;
	int new_mb = sent_bytes>>20;
	if (new_mb>old_mb)
	{
		fprintf(stderr, "sent cumulative %dMB\n", new_mb);
	}
}
