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

	assert(argc==3);
	const char *connectaddr_arg = argv[1];
	const char *connectport_arg = argv[2];

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock>=0);

	struct sockaddr_in connect_addr;
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = htons(atoi(connectport_arg));
	rc = inet_aton(connectaddr_arg, &connect_addr.sin_addr);
	assert(rc);
	
	char reqbuf[1024];
	rc = sendto(sock, reqbuf, sizeof(reqbuf), 0,
		(struct sockaddr *) &connect_addr, sizeof(connect_addr));
	assert(rc==sizeof(reqbuf));

	static char recvbuf[32900];
	size_t recv_size = recv(sock, recvbuf, sizeof(recvbuf), 0);
	fprintf(stderr, "Received %d bytes\n", recv_size);

	return 0;
}
