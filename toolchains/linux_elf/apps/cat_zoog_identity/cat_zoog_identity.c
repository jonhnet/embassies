#include <string.h>
#include <stdio.h>

int main()
{
	char buf[100];
	memset(buf, 0, sizeof(buf));
	int rc;
	const char *path = "/etc/zoog_identity";
	FILE *fp = fopen(path, "r");
	if (fp==0)
	{
		perror("fopen");
		return -1;
	}
	rc = fread(buf, 1, sizeof(buf), fp);
	if (rc<=0)
	{
		perror("fread");
		return -1;
	}
	fclose(fp);
	if (rc>0)
	{
		fprintf(stderr, "%s contains '%s'.\n", path, buf);
	}
	else
	{
		fprintf(stderr, "fread failure.\n");
	}
	return 0;
}
