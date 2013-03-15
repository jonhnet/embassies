#include <stdio.h>
#include <assert.h>

#include "ZKeyPair.h"
#include "ambient_malloc.h"
#include "standard_malloc_factory.h"

int main(int argc, char **argv) {
	ambient_malloc_init(standard_malloc_factory_init());

	assert(argc==3);
	char *in_pair_name = argv[1];
	char *out_pub_name = argv[2];

	uint8_t i_buf[8000];
	FILE *ifp = fopen(in_pair_name, "rb");
	int i_size = fread(i_buf, 1, sizeof(i_buf), ifp);
	fclose(ifp);

	assert(i_size>0);

	ZKeyPair *key_pair = new ZKeyPair(i_buf, i_size);
	ZPubKey *pub_key = key_pair->getPubKey();

	uint8_t o_buf[8000];
	assert(pub_key->size() <= sizeof(o_buf));
	pub_key->serialize(o_buf);
	FILE *ofp = fopen(out_pub_name, "wb");
	int o_size = fwrite(o_buf, 1, pub_key->size(), ofp);
	fclose(ofp);

	assert(o_size==(int)pub_key->size());

	ZPubKey *pub_key_reloaded = new ZPubKey(o_buf, pub_key->size());
	(void) pub_key_reloaded;

	return 0;
}
