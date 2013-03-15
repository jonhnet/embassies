#include <malloc.h>
#include <string.h>
#include "SockaddrHashable.h"
#include "hash_table.h"

SockaddrHashable::SockaddrHashable(const char *sockaddr, int sockaddr_len)
{
	this->sockaddr = (char*) malloc(sockaddr_len);
	memcpy(this->sockaddr, sockaddr, sockaddr_len);
	this->sockaddr_len = sockaddr_len;
}

SockaddrHashable::~SockaddrHashable()
{
	free(sockaddr);
}

uint32_t SockaddrHashable::hash()
{
	return hash_buf(sockaddr, sockaddr_len);
}

int SockaddrHashable::cmp(Hashable *v_other)
{
	SockaddrHashable *other = (SockaddrHashable *) v_other;
	return cmp_bufs(
		sockaddr, sockaddr_len,
		other->sockaddr, other->sockaddr_len);
}
