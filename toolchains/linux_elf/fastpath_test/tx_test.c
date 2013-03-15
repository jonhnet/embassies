#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>

int main()
{
	int rc;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	int one = 1;
	rc = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
	if (rc!=0) { perror("setsockopt"); }
	assert(rc==0);

	struct sockaddr_in myaddr;
	uint32_t myaddr_sz = sizeof(myaddr);
	rc = getsockname(sock, (struct sockaddr *) &myaddr, &myaddr_sz);
	if (rc!=0) { perror("getsockname"); }
	assert(rc==0);
	fprintf(stderr, "my addr %08x :%d\n",
		myaddr.sin_addr.s_addr, myaddr.sin_port);

	struct sockaddr_in bindaddr;
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = 3456;
	//bindaddr.sin_addr.s_addr = INADDR_BROADCAST;
	bindaddr.sin_addr.s_addr = 0xffffff7f;
	rc = connect(sock, (const struct sockaddr*) &bindaddr, sizeof(bindaddr));

	if (rc!=0) { perror("connect"); }
	assert(rc==0);

	while (1)
	{
		char buf[800];
		memset(buf, 'c', sizeof(buf));
		int size = send(sock, buf, sizeof(buf), 0);
		if (size<0)
		{
			fprintf(stderr, "Error on send %d.\n", errno);
			perror("send");
			break;
		}
		fprintf(stderr, "Send sent %d bytes.\n", size);
	}

	return -1;
}
