#include <assert.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

// Wherein jonh learns about Linux' "abstract namespace" for
// PF_FILE/PF_UNIX sockets.

int main()
{
	int rc;
	int sockfd = socket(PF_FILE, SOCK_STREAM, 0);
	assert(sockfd>=0);

	struct sockaddr_un saddr;
	saddr.sun_family = PF_UNIX;
	strcpy(saddr.sun_path+1, "/tmp/dbus-CAaHmZZcn2");
	saddr.sun_path[0] = '\0';
	int len = 23;
	rc = connect(sockfd, (struct sockaddr*) &saddr, len);
	assert(rc==0);
	return 0;
}
