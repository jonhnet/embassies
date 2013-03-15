#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>

const int test_port = 2048;
const int buf_size = 65000;

int checksum(const char *buffer, int len)
{
	int v = 0;
	int i;
	for (i=0; i<len; i++)
	{
		v = v*131 + buffer[i];
	}
	return v;
}

typedef struct {
	int fd;
} Receiver;

void receiver_init(Receiver *rxr)
{
	int rc;
	rxr->fd = socket(AF_INET6, SOCK_DGRAM, 0);

	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(test_port);
	memset(&addr.sin6_addr, 0, sizeof(addr.sin6_addr));
	addr.sin6_addr.s6_addr[15] = 1;
	addr.sin6_scope_id = 0;

	rc = bind(rxr->fd, (const struct sockaddr*) &addr, sizeof(addr));
	assert(rc==0);
}

void receiver_receive(Receiver *rxr, int expect_checksum)
{
	int rc;
	char *buffer = malloc(buf_size);
	rc = recv(rxr->fd, buffer, buf_size, 0);
	assert(rc==buf_size);
	int got_checksum = checksum(buffer, buf_size);
	assert(expect_checksum==got_checksum);
}

typedef struct {
	int fd;
} Transmitter;

void transmitter_init(Transmitter *txr)
{
	txr->fd = socket(AF_INET6, SOCK_DGRAM, 0);
	assert(txr->fd>=0);
}

void transmitter_send(Transmitter *txr, int *expect_checksum)
{
	char *buffer = malloc(buf_size);
	memset(buffer, 6, buf_size);

	*expect_checksum = checksum(buffer, buf_size);

	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(test_port);
	memset(&addr.sin6_addr, 0, sizeof(addr.sin6_addr));
	addr.sin6_addr.s6_addr[15] = 1;
	addr.sin6_scope_id = 0;

	int rc;
	rc = sendto(txr->fd, buffer, buf_size, 0,
		(const struct sockaddr*) &addr, sizeof(addr));
	if (rc!=buf_size)
	{
		perror("sendto");
	}
	assert(rc==buf_size);
	fprintf(stderr, "rc=%d\n", rc);
}

int main()
{
	int expect_checksum;
	Receiver rxr;
	receiver_init(&rxr);

	Transmitter txr;
	transmitter_init(&txr);
	transmitter_send(&txr, &expect_checksum);

	receiver_receive(&rxr, expect_checksum);



	return 0;
}
