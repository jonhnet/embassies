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

PosixSocket::PosixSocket(UDPEndpoint* local, UDPEndpoint* remote, MallocFactory *mf, bool want_timeouts, SocketFactoryType::Type type)
{
	int rc;

	this->_want_timeouts = want_timeouts;
	this->type = type;

	this->mf = mf;
	int socket_type = type==SocketFactoryType::UDP ? SOCK_DGRAM : SOCK_STREAM;
	int bind_fd = socket(PF_INET, socket_type, 0);
	lite_assert(bind_fd >=0);

	outgoing = (local==NULL);
	lite_assert(!outgoing || remote!=NULL);

	struct sockaddr origin_saddr;
	if (outgoing)
	{
		lite_assert(sizeof(struct sockaddr_in) <= sizeof(origin_saddr));
		struct sockaddr_in *sin = (struct sockaddr_in *) &origin_saddr;
		sin->sin_family = PF_INET;
		sin->sin_addr.s_addr = 0;
		sin->sin_port = 0;
	}
	else
	{
		SOCKADDR_FROM_EP(origin_saddr, local);
	}

	if (type==SocketFactoryType::TCP)
	{
		int yes=1;
		rc = setsockopt(bind_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		lite_assert(rc==0);
	}

	if (outgoing && type==SocketFactoryType::TCP)
	{
		// outgoing TCP? Don't bind, connect later.
	}
	else
	{
		rc = bind(bind_fd, &origin_saddr, sizeof(origin_saddr));
		if (rc!=0)
		{
			fprintf(stderr, "Couldn't bind to socket. I'm going to go out on a limb and guess that you ran zftp_local_cache against tunid without starting a monitor first to open the tunnel?\n");
			lite_assert(rc==0);
		}
	}

	if (type==SocketFactoryType::UDP)
	{
		sock_fd = bind_fd;
	}
	else
	{
		tcp_listen_fd = bind_fd;
		if (outgoing)
		{
			tcp_connect(remote);
			tcp_remote = *remote;
		}
		else
		{
			rc = listen(tcp_listen_fd, 1);
			lite_assert(rc==0);
			sock_fd = -1;
		}
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

void PosixSocket::tcp_connect(UDPEndpoint* remote)
{
	int rc;
	struct sockaddr remote_saddr;
	SOCKADDR_FROM_EP(remote_saddr, remote);
	rc = connect(tcp_listen_fd, &remote_saddr, sizeof(remote_saddr));
	lite_assert(rc == 0);
	sock_fd = tcp_listen_fd;
	fprintf(stderr, "Connected new TCP connection.\n");
}

bool PosixSocket::zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb, uint32_t final_len)
{
	// (TCP case) Assume that we never try to send to different addresses,
	// so that if we're already connected, the packet must be going
	// the same place as the existing connection.

	//SFPMessage *sfpm = (SFPMessage *) zcb;
	struct sockaddr remote_saddr;
	SOCKADDR_FROM_EP(remote_saddr, remote);

	int rc;
	rc = ::sendto(sock_fd, zcb->data(), final_len, 0,
		&remote_saddr, sizeof(remote_saddr));
	bool result = (rc>=0 && (((uint32_t) rc)==final_len));
	return result;
}

void PosixSocket::zc_release(ZeroCopyBuf *zcb)
{
	delete zcb;
}

void PosixSocket::tcp_accept()
{
	lite_assert(type == SocketFactoryType::TCP);
	lite_assert(!outgoing);

	struct sockaddr from_saddr;
	socklen_t saddr_len = sizeof(from_saddr);

	int rc;
	rc = accept(tcp_listen_fd, &from_saddr, &saddr_len);
	lite_assert(rc >= 0);
	sock_fd = rc;
	fprintf(stderr, "Accepted new TCP connection.\n");

	lite_assert(saddr_len == sizeof(from_saddr));
	EP_FROM_SOCKADDR(&tcp_remote, from_saddr);
}

void PosixSocket::tcp_close_client()
{
	fprintf(stderr, "Dropped client fd %d\n", sock_fd);
	close(sock_fd);
	sock_fd = -1;
}

ZeroCopyBuf *PosixSocket::recvfrom(UDPEndpoint *out_remote)
{
	struct sockaddr from_saddr;
	uint8_t buf[65536];
	// hardcoded packet size limit. posix can go higher with crazy mtus,
	// but not very much higher.

	while (true)
	{
		if (sock_fd==-1)
		{
			tcp_accept();
		}

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
		if (rc==0)
		{
			lite_assert(type==SocketFactoryType::TCP);
			tcp_close_client();
			continue;
		}
		if (rc<0)
		{
			return false;
		}
		ZeroCopyBuf *zcb = new ZeroCopyBuf_Memcpy(mf, buf, rc);

		if (out_remote!=NULL)
		{
			if (type==SocketFactoryType::TCP)
			{
				lite_assert(saddr_len == 0);
				*out_remote = tcp_remote;
			}
			else
			{
				lite_assert(saddr_len == sizeof(from_saddr));
				EP_FROM_SOCKADDR(out_remote, from_saddr);
			}
		}
		return zcb;
	}
}

//////////////////////////////////////////////////////////////////////////////

SocketFactory_Posix::SocketFactory_Posix(MallocFactory *mf, SocketFactoryType::Type type)
{
	this->mf = mf;
	this->type = type;
}

AbstractSocket *SocketFactory_Posix::new_socket(UDPEndpoint *local, UDPEndpoint *remote, bool want_timeouts)
{
	return new PosixSocket(local, remote, mf, want_timeouts, type);
}
