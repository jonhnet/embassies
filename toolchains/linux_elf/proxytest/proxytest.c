#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int main(void)
{
	while (1)
	{
		sleep(1);

#define TRYAGAIN(m)	{ fprintf(stdout, "ERROR %s\n", m); perror("error"); continue; }

		int rc;
		int s;
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s<0) { TRYAGAIN("socket"); }

		struct sockaddr_in sai;
		sai.sin_family = AF_INET;
		sai.sin_port = htons(80);
		inet_aton("172.31.40.21", &sai.sin_addr);
		rc = connect(s, (struct sockaddr*) &sai, sizeof(sai));
		if (rc!=0) { TRYAGAIN("connect"); }

		FILE *sfp = fdopen(s, "r+");
		if (sfp==NULL) { TRYAGAIN("fdopen"); }

		fprintf(sfp, "HTTP/1.1 GET http://www.jonh.net/\nHost: www.jonh.net\n\n");
		fflush(sfp);
		char buf[800];
		while (1)
		{
			char *msg = fgets(buf, sizeof(buf), sfp);
			if (msg==NULL)
			{
				break;
			}
			fprintf(stdout, ":: %s", msg);
			fflush(stdout);
		}
		fclose(sfp);
		fprintf(stdout, "Success.\n");
	}
	int rc = mkdir("/home/webguest/docs", 0777);
	fprintf(stdout, "mkdir() said %d\n", rc);
	FILE *fp = fopen("/home/webguest/docs/test.txt", "r");
	if (fp!=NULL)
	{
        char buf[100];
		rc = fread(buf, 1, 99, fp);
        if (rc > 0) {
            buf[rc] = '\0';
            fprintf(stdout, "Contents of file is \"%s\"\n", buf);
        }
        else {
            fprintf(stdout, "No data read from file\n");
        }
		fclose(fp);
	}
	else
	{
		fprintf(stdout, "Could not open file (%d)\n", errno);
	}
	return 0;
}
