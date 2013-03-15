#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, const char **argv)
{
	int sock;
	int rc;

	if (argc!=5)
	{
		fprintf(stderr, "Usage: %s <listen_addr> <listen_port> <reply_size> <reply_count>\n", argv[0]);
		exit(1);
	}
	assert(argc==5);
	const char *listenaddr = argv[1];
	const char *listenport = argv[2];
	const char *reply_size_arg = argv[3];
	const char *reply_count_arg = argv[4];

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock>=0);

	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(atoi(listenport));
	rc = inet_aton(listenaddr, &bind_addr.sin_addr);
	assert(rc);
	
	rc = bind(sock, (const struct sockaddr *) &bind_addr, sizeof(bind_addr));
	assert(rc==0);

	struct sockaddr_in recv_addr;
	char recvbuf[1024];

	typedef struct {
		uint32_t number;
		uint32_t total;
	} Signal;
	int sendbuf_size = atoi(reply_size_arg);
	char *sendbuf = alloca(sendbuf_size);
	assert(sizeof(uint32_t) <= sendbuf_size);
	Signal *signal = (Signal*) sendbuf;
//	memset(sendbuf, 34, sendbuf_size);
//	sendbuf[sendbuf_size-1] = 33;

	// imprint a pattern that would be hard to reorder without us noticing.
	int i,j;
	for (i=sizeof(Signal), j=0; i<sendbuf_size; i++, j=(j+1)%217)
	{
		sendbuf[i] = j;
	}

	int reply_count = atoi(reply_count_arg);

	while (1)
	{
		size_t recv_addr_size = sizeof(recv_addr);
		rc = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
			(struct sockaddr *) &recv_addr, &recv_addr_size);
		assert(rc>=0);

		int counti;
		for (counti=0; counti<reply_count; counti++)
		{
			signal->number = counti;
			signal->total = reply_count;
			rc = sendto(sock, sendbuf, sendbuf_size, 0,
				(struct sockaddr *) &recv_addr, recv_addr_size);
			assert(rc==sendbuf_size);
		}
	}

	return 0;
}
