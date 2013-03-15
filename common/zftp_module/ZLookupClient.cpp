#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif // _WIN32

#include "LiteLib.h"
#include "zftp_hash_lookup_protocol.h"
#include "xax_network_utils.h"
#include "zlc_util.h"
#include "ZLookupClient.h"
#include "zemit.h"

ZLookupClient::ZLookupClient(UDPEndpoint *origin_lookup, SocketFactory *sockf, SyncFactory *syncf, ZLCEmit *ze)
{
	this->origin_lookup = origin_lookup;
	this->ze = ze;
	this->sockf = sockf;
	nonce = 100;
	mutex = syncf->new_mutex(false);

	sock = NULL;
}

void ZLookupClient::_open_socket()
{
	if (sock==NULL)
	{
		ZLC_COMPLAIN(ze, "ZLookupClient breaks open a new socket.\n");
		sock = sockf->new_socket(
			sockf->get_inaddr_any(origin_lookup->ipaddr.version), true);
	}
	lite_assert(sock!=0);
}

hash_t ZLookupClient::lookup_url(const char *url)
{
	mutex->lock();

	_open_socket();

	nonce++;

	int url0len = lite_strlen(url)+1;
	int lookup_len = sizeof(ZFTPHashLookupRequestPacket)+url0len;
	ZFTPHashLookupRequestPacket *lookup =
		(ZFTPHashLookupRequestPacket *) alloca(lookup_len);
	lookup->nonce = z_htong(nonce, sizeof(lookup->nonce));
	lookup->url_hint_len = z_htons(url0len);
	char *url_hint = (char*) &lookup[1];
	lite_memcpy(url_hint, url, url0len);

	ZeroCopyBuf *zcb;
	ZFTPHashLookupReplyPacket *reply = NULL;
	bool retransmissions = false;
	bool transmit = true;
	while (true)
	{
		reply = NULL;
		if (transmit)
		{
			bool rc;
			rc = sock->sendto(origin_lookup, lookup, lookup_len);
			lite_assert(rc);
			transmit = false;
		}

		zcb = sock->recvfrom(NULL);
		if (zcb==NULL)
		{
			// close the sock later, to keep duplicate replies from piling up.
			retransmissions = true;

			transmit = true;
			continue;
		}

		lite_assert(zcb->len()==sizeof(ZFTPHashLookupReplyPacket));

		reply = (ZFTPHashLookupReplyPacket *) zcb->data();
		if (Z_NTOHG(reply->nonce)!=nonce)
		{
			ZLC_COMPLAIN(ze, "wrong nonce (old?)\n");
			// don't retransmit; just wait again.
			delete zcb;
			continue;
		}
		break;
	}
	lite_assert(zcb!=NULL);	// we retry forever above.
	lite_assert(Z_NTOHG(reply->hash_len)==sizeof(hash_t));
	//lite_assert(!is_zero_hash(&reply.hash));

	if (retransmissions)
	{
		delete sock;
		sock = NULL;
	}

	mutex->unlock();

	hash_t result = reply->hash;
	delete zcb;
	return result;
}


