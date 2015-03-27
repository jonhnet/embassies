#include <stdio.h>
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

bool CounterSocket::zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb, uint32_t final_len)
{
	sfc->_count_send(final_len);
	return us->zc_send(remote, zcb, final_len);
}
void CounterSocket::zc_release(ZeroCopyBuf *zcb)
{
	us->zc_release(zcb);
}

ZeroCopyBuf *CounterSocket::recvfrom(UDPEndpoint *out_remote)
{
	ZeroCopyBuf *zcb = us->recvfrom(out_remote);
	if (zcb!=NULL)
	{
		sfc->_count_received(zcb->len());
	}
	return zcb;
}

//////////////////////////////////////////////////////////////////////////////

SocketFactory_Counter::SocketFactory_Counter(SocketFactory* usf)
	: usf(usf),
	  sent_bytes(0),
	  received_bytes(0)
{}

AbstractSocket *SocketFactory_Counter::new_socket(UDPEndpoint* local, UDPEndpoint* remote, bool want_timeouts)
{
	return new CounterSocket(usf->new_socket(local, remote, want_timeouts), this);
}

void SocketFactory_Counter::_count_send(int _sent_bytes)
{
//	int old_mb = sent_bytes>>20;
	this->sent_bytes += _sent_bytes;
/*
	int new_mb = sent_bytes>>20;
	if (new_mb>old_mb)
	{
		fprintf(stderr, "sent cumulative %dMB\n", new_mb);
	}
*/
//	fprintf(stderr, "SocketFactory_Counter: sent(%d) => %d\n", _sent_bytes, this->sent_bytes);
}

void SocketFactory_Counter::_count_received(int _received_bytes)
{
	this->received_bytes += _received_bytes;
//	fprintf(stderr, "SocketFactory_Counter: recv(%d) => %d\n", _received_bytes, this->received_bytes);
}

void SocketFactory_Counter::reset_stats()
{
	sent_bytes = 0;
	received_bytes = 0;
//	fprintf(stderr, "SocketFactory_Counter: reset\n");
}
