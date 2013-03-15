#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include "crypto.h"
#include "ZCert.h"
#include "hash_to_hex.h"

#include "ambient_malloc.h"
#include "standard_malloc_factory.h"

int main(int argc, char **argv)
{
	ambient_malloc_init(standard_malloc_factory_init());

	assert(argc==2);
	char *path = argv[1];
	FILE *fp = fopen(path, "rb");
	if (fp==NULL)
	{
		perror("open");
		return -1;
	}
	uint32_t buflen = 1<<20;
	uint8_t *buf = (uint8_t*) malloc(buflen);
	int rc;
	rc = fread(buf, 1, buflen, fp);
	assert(rc < (int) buflen);	// else I underestimated
	fclose(fp);

	ZCertChain* zcc = new ZCertChain(buf, rc);
	ZCert* zc = zcc->getMostSpecificCert();

	hash_t pub_key_hash;
	ZPubKey *k = zc->getEndorsedKey();
	uint8_t *kbuf = (uint8_t*) malloc(k->size());
	k->serialize(kbuf);
	zhash(buf, k->size(), &pub_key_hash);
	free(kbuf);
	
	char hexbuf[200];
	hash_to_hex(pub_key_hash, hexbuf);
	fprintf(stdout, "%s\n", hexbuf);

	return 0;
}
