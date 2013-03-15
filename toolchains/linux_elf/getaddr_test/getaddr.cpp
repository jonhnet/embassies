#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

int main() {
	struct addrinfo hints;
	struct addrinfo *result;
	int s;

	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	char host[] = "parno.net";
	char port[] = "80";

	s = getaddrinfo(host, port, &hints, &result);

	if (s != 0) {
		 fprintf(stderr, "Failed, err code: %d\n", s); //"getaddrinfo: %s\n", gai_strerror(s));
		 exit(1);
	}
	fprintf(stderr, "Lookup succeeded!\n");
}
