#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include "LiteLib.h"
#include "SocketFactory_Posix.h"
#include "sockaddr_util.h"
#include "ZeroCopyBuf_Memcpy.h"

PosixSocket::PosixSocket(UDPEndpoint *local, MallocFactory *mf, bool want_timeouts)
{
	this->_want_timeouts = want_timeouts;

	this->mf = mf;
	sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	lite_assert(sock_fd >=0);

	struct sockaddr origin_saddr;
	if (local!=NULL)
	{
		SOCKADDR_FROM_EP(origin_saddr, local);
	}
	else
	{
		lite_assert(sizeof(struct sockaddr_in) <= sizeof(origin_saddr));
		struct sockaddr_in *sin = (struct sockaddr_in *) &origin_saddr;
		sin->sin_family = PF_INET;
		sin->sin_addr.s_addr = 0;
		sin->sin_port = 0;
	}

	int rc = bind(sock_fd, &origin_saddr, sizeof(origin_saddr));
	if (rc!=0)
	{
		fprintf(stderr, "Couldn't bind to socket. I'm going to go out on a limb and guess that you ran zftp_local_cache against tunid without starting a monitor first to open the tunnel?\n");
		lite_assert(rc==0);
	}
}

PosixSocket::~PosixSocket()
{
	close(sock_fd);
}

class SFPMessage : public ZeroCopyBuf_Memcpy {
public:
	SFPMessage(MallocFactory *mf, uint32_t payload_len)
		: ZeroCopyBuf_Memcpy(mf, payload_len) {}
private:
};

ZeroCopyBuf *PosixSocket::zc_allocate(uint32_t payload_len)
{
	return new SFPMessage(mf, payload_len);
}

bool PosixSocket::zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb)
{
	//SFPMessage *sfpm = (SFPMessage *) zcb;
	struct sockaddr remote_saddr;
	SOCKADDR_FROM_EP(remote_saddr, remote);

	int rc;
	rc = ::sendto(sock_fd, zcb->data(), zcb->len(), 0,
		&remote_saddr, sizeof(remote_saddr));
	bool result = (rc>=0 && (((uint32_t) rc)==zcb->len()));
	return result;
}

void PosixSocket::zc_release(ZeroCopyBuf *zcb)
{
	delete zcb;
}

ZeroCopyBuf *PosixSocket::recvfrom(UDPEndpoint *out_remote)
{
	struct sockaddr from_saddr;
	uint8_t buf[65536];
	// hardcoded packet size limit. posix can go higher with big mtus,
	// but not very much higher.

	if (_want_timeouts)
	{
		struct pollfd fd;
		fd.fd = sock_fd;
		fd.events = POLLIN;
		int timeout_ms = 500;
		int rc = poll(&fd, 1, timeout_ms);
		if (rc==0)
		{
			return NULL;
		}
		lite_assert(rc==1);
	}

	socklen_t saddr_len = sizeof(from_saddr);
	int rc = ::recvfrom(sock_fd, buf, sizeof(buf), 0,
		&from_saddr, &saddr_len);
	if (rc<0)
	{
		return false;
	}
	ZeroCopyBuf *zcb = new ZeroCopyBuf_Memcpy(mf, buf, rc);

	if (out_remote!=NULL)
	{
		lite_assert(saddr_len == sizeof(from_saddr));
		EP_FROM_SOCKADDR(out_remote, from_saddr);
	}
	return zcb;
}

//////////////////////////////////////////////////////////////////////////////

SocketFactory_Posix::SocketFactory_Posix(MallocFactory *mf)
{
	this->mf = mf;
}

AbstractSocket *SocketFactory_Posix::new_socket(UDPEndpoint *local, bool want_timeouts)
{
	return new PosixSocket(local, mf, want_timeouts);
}
