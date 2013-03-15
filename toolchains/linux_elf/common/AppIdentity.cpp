#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "AppIdentity.h"

#ifndef ZOOG_ROOT
// ugly expedient for libwebkit build, for which we haven't yet found a sane
// way to stuff ZOOG_ROOT into its makefile.
#include "zoog_root.h"
#endif

void AppIdentity::load_identity_name()
{
	const char *identity_path = "/zoog/env/identity";
	FILE *fp = fopen(identity_path, "r");
	if (fp==NULL) {
		fprintf(stderr, "No file present at %s\n", identity_path);
		assert(false);
	} 
	int rc = fread(identity_name, 1, sizeof(identity_name)-1, fp);
	fclose(fp);
	identity_name[rc]='\0';
	fprintf(stderr, "Read: '%s'\n", identity_name);
}

void AppIdentity::load_cert_chain()
{
	if (certChain!=NULL)
	{
		return;
	}

	char cert_path[2000];
	snprintf(cert_path, sizeof(cert_path), ZOOG_ROOT "/toolchains/linux_elf/crypto/certs/%s.chain", identity_name);
	FILE *cfp = fopen(cert_path, "rb");
	if (cfp==NULL)
	{
		fprintf(stderr, "Unable to open %s\n", cert_path);
		assert(false);
	}
	fseek(cfp, 0, SEEK_END);
	int len = ftell(cfp);
	fseek(cfp, 0, SEEK_SET);
	char *buf = (char*) malloc(len);
	int rc;
	rc = fread(buf, len, 1, cfp);
	assert(rc==1);	// hey, I just measured len!
	fclose(cfp);

	this->certChain = new ZCertChain((uint8_t*) buf, len);
	assert(certChain !=NULL);
	assert(get_pub_key() != NULL);
	assert(get_domain_name() != NULL);
}

AppIdentity::AppIdentity()
	: certChain(NULL),
	  hash_valid(false)
{
	load_identity_name();
}

AppIdentity::AppIdentity(const char *_identity_name)
	: certChain(NULL),
	  hash_valid(false)
{
	strncpy(this->identity_name, _identity_name, sizeof(this->identity_name));
}

AppIdentity::~AppIdentity()
{
	delete certChain;
}

const char *AppIdentity::get_identity_name()
{
	return identity_name;
}

ZCert *AppIdentity::get_cert()
{
	load_cert_chain();
	return certChain->getMostSpecificCert();
}

ZCertChain *AppIdentity::get_cert_chain()
{
	load_cert_chain();
	return certChain;
}

ZPubKey *AppIdentity::get_pub_key()
{
	load_cert_chain();
	return get_cert()->getEndorsedKey();
}

hash_t AppIdentity::get_pub_key_hash()
{
	load_cert_chain();
	if (!hash_valid)
	{
		ZPubKey *k = get_pub_key();
		uint8_t *buf = (uint8_t*) malloc(k->size());
		k->serialize(buf);
		zhash(buf, k->size(), &pub_key_hash);
		free(buf);
		hash_valid = true;
	}
	return pub_key_hash;
}

const char *AppIdentity::get_domain_name()
{
	load_cert_chain();
	return get_pub_key()->getOwner()->toString();
}
