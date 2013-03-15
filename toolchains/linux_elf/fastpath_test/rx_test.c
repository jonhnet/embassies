#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>

int main()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in bindaddr;
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = 3456;
	bindaddr.sin_addr.s_addr = INADDR_ANY;
	int rc = bind(sock, (const struct sockaddr*) &bindaddr, sizeof(bindaddr));

	assert(rc==0);

	while (1)
	{
		char buf[800];
		int size = recv(sock, buf, sizeof(buf), 0);
		if (size<0)
		{
			fprintf(stderr, "Error on recv %d.\n", errno);
			break;
		}
		fprintf(stderr, "Recv sees %d bytes.\n", size);
	}

	return -1;
}
