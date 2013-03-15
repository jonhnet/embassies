#include "avl2.h"
#include "FragmentReassembly.h"
#include "LiteLib.h"
#include "zemit.h"
#include "xax_network_utils.h"

typedef uint16_t FragmentKey;

class FragmentElt
{
private:
	FragmentKey key;
	ZeroCopyBuf *zcb;
	IPInfo info;

public:
	FragmentElt(ZeroCopyBuf *zcb, IPInfo *info)
		: key(info->fragment_offset), zcb(zcb), info(*info) {}
	~FragmentElt()
		{ delete zcb; }

	uint8_t *header() { return zcb->data(); }
	uint32_t header_size() { return info.payload_offset; }

	FragmentKey getKey() { return key; }
	FragmentKey start() { return key; }
	FragmentKey size() { return info.payload_length; }
	FragmentKey end() { return key+size(); }
	uint8_t *data() { return zcb->data()+info.payload_offset; }
	bool equals(FragmentElt *other)
		{ return start()==other->start() && size()==other->size(); }
	IPInfo *get_info() { return &info; }
};

typedef AVLTree<FragmentElt,FragmentKey> FragmentTree;

class PartialPacket
	: public ZeroCopyBuf
{
private:
	uint16_t ident;
	bool is_key;
	MallocFactory *mf;
	ZLCEmit *ze;

	bool have_last_fragment;
	FragmentKey next_gap;

	FragmentTree *fragments;
	uint8_t *assembled_data;
	uint32_t assembled_len;
	IPInfo info;

	void assemble_packet();

public:
	PartialPacket(uint16_t ident, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze);
	PartialPacket(uint16_t ident);	// key ctor
	~PartialPacket();

	bool ingest_fragment(ZeroCopyBuf *zcb, IPInfo *info);
		// returns true if this fragment completes the packet

	// view of assembled packet
	uint8_t *data();
	uint32_t len();
	IPInfo *get_info();

	static uint32_t hash(const void *datum);
	static int cmp(const void *a, const void *b);
};

PartialPacket::PartialPacket(uint16_t ident, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze)
	: ident(ident),
	  is_key(false),
	  mf(mf),
	  ze(ze),
	  have_last_fragment(false),
	  next_gap(0),
	  fragments(new FragmentTree(sf)),
	  assembled_data(NULL)
{
}

PartialPacket::PartialPacket(uint16_t ident)
	: ident(ident),
	  is_key(true),
	  mf(NULL),
	  fragments(NULL),
	  assembled_data(NULL)
{
}

PartialPacket::~PartialPacket()
{
	if (!is_key)
	{
		if (assembled_data!=NULL)
		{
			mf_free(mf, assembled_data);
		}

		while (true)
		{
			FragmentElt *fragment = fragments->findMin();
			if (fragment==NULL) { break; }
			fragments->remove(fragment);
			delete fragment;
		}
		delete fragments;
	}
}

void PartialPacket::assemble_packet()
{
	FragmentElt *firstFragment = fragments->findMin();
	assembled_len = firstFragment->header_size();
	FragmentElt *fragment = firstFragment;
	while (fragment!=NULL)
	{
		assembled_len += fragment->size();
		ZLC_CHATTY(ze, "Add fragment %d..%d\n",, fragment->start(), fragment->end());
		fragment = fragments->findFirstGreaterThan(fragment);
	}

	assembled_data = (uint8_t*) mf_malloc(mf, assembled_len);
	lite_memcpy(assembled_data,
		firstFragment->header(),
		firstFragment->header_size());

	fragment = firstFragment;
	uint32_t out_offset = firstFragment->header_size();
	while (fragment!=NULL)
	{
		lite_memcpy(&assembled_data[out_offset], fragment->data(), fragment->size());
		out_offset += fragment->size();
		fragment = fragments->findFirstGreaterThan(fragment);
	}

	// Massage the copied-in header to reflect the assembled packet,
	// and parse that for delivery.
	IP4Header *ip4 = (IP4Header*) assembled_data;
	ip4->total_length = z_htong(assembled_len, sizeof(ip4->total_length));
	ip4->flags_and_fragment_offset = 0;
	decode_ip_packet(&info, assembled_data, assembled_len);
}

bool PartialPacket::ingest_fragment(ZeroCopyBuf *zcb, IPInfo *info)
{
	lite_assert(!is_key && assembled_data==NULL);

	FragmentElt *newFragment = new FragmentElt(zcb, info);

	FragmentElt *priorFragment = fragments->findFirstLessThanOrEqualTo(newFragment);
	lite_assert(
		   priorFragment==NULL
		|| priorFragment->end() <= newFragment->start()
		|| priorFragment->equals(newFragment));

	FragmentElt *nextFragment = fragments->findFirstGreaterThan(newFragment);
	lite_assert(
		   nextFragment == NULL
		|| newFragment->end() <= nextFragment->start());

	if (!info->more_fragments)
	{
		lite_assert(nextFragment==NULL);
		this->have_last_fragment = true;
	}

	bool rc = fragments->insert(newFragment);
	lite_assert(rc);
	
	if (next_gap == newFragment->start())
	{
		// advance next_gap past contiguous fragments to find the
		// next offset at which we don't have a fragment.
		// This may be at the end of the packet; that is, at the end()
		// of the last fragment -- and we're done.
		// We'll detect that case in the next block.
		FragmentElt *next_frag = newFragment;
		while (next_frag!=NULL)
		{
			next_gap = newFragment->end();
			next_frag = fragments->findFirstGreaterThan(next_frag);
		}
	}

	if (this->have_last_fragment)
	{
		FragmentElt *lastFragment = fragments->findMax();
		lite_assert(next_gap <= lastFragment->end());
		if (next_gap == lastFragment->end())
		{
			// packet is done; assemble it.
			assemble_packet();
			return true;
		}
	}

	return false;
}

uint8_t *PartialPacket::data()
{
	lite_assert(!is_key && assembled_data!=NULL);
	// TODO ZeroCopy for large values of zero. :v) Would need to
	// improve the customer-facing interface to avoid this assembly.
	return assembled_data;
}

uint32_t PartialPacket::len()
{
	lite_assert(!is_key && assembled_data!=NULL);
	return assembled_len;
}

IPInfo *PartialPacket::get_info()
{
	lite_assert(!is_key && assembled_data!=NULL);
	return &info;
}

uint32_t PartialPacket::hash(const void *datum)
{
	return ((PartialPacket*) datum)->ident;
}

int PartialPacket::cmp(const void *a, const void *b)
{
	return ((PartialPacket*) a)->ident - ((PartialPacket*) b)->ident;
}

//////////////////////////////////////////////////////////////////////////////

FragmentReassembly::FragmentReassembly(PacketDispatcherIfc *dispatcher, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze)
	: dispatcher(dispatcher),
	  mf(mf),
	  sf(sf),
	  ze(ze)
{
	ZLC_CHATTY(ze, "FragmentReassembly ctor");
	hash_table_init(&_ht0, mf, PartialPacket::hash, PartialPacket::cmp);
	hash_table_init(&_ht1, mf, PartialPacket::hash, PartialPacket::cmp);
	h_new = &_ht0;
	h_old = &_ht1;
}

void FragmentReassembly::remove_func(void *v_this, void *datum)
{
	PartialPacket *packet = (PartialPacket *) datum;
	delete packet;
}

void FragmentReassembly::tidy()
{
	while (h_new->count + h_old->count > MAX_PARTIAL_PACKETS)
	{
		hash_table_remove_every(h_old, remove_func, NULL);
		HashTable *tmp = h_new;
		h_new = h_old;
		h_old = tmp;
	}
}

void FragmentReassembly::handle_fragment(ZeroCopyBuf *zcb, IPInfo *info)
{
	ZLC_CHATTY(ze, "FragmentReassembly::handle_fragment");
	HashTable *home = NULL;
	PartialPacket key(info->identification);

	home = h_new;
	PartialPacket *partial_packet = (PartialPacket *)
		hash_table_lookup(home, &key);
	if (partial_packet == NULL)
	{
		home = h_old;
		partial_packet = (PartialPacket *) hash_table_lookup(home, &key);
	}
	if (partial_packet == NULL)
	{
		tidy();
			// NB tidy comes before insert, so we don't write down the 'home'
			// value only to have the packet move to the other hash table on
			// us.

		home = h_new;
		partial_packet = new PartialPacket(info->identification, mf, sf, ze);
		hash_table_insert(home, partial_packet);
		ZLC_CHATTY(ze, "created new partial packet (id %x)\n",, info->identification);
	}
	else
	{
		ZLC_CHATTY(ze, "using existing partial packet from %s (id %x)\n",,
			home==h_old ? "h_old" : "h_new",
			info->identification);
	}

	if (partial_packet->ingest_fragment(zcb, info))
	{
		ZLC_CHATTY(ze, "Completed packet len %d\n",, partial_packet->len());
		dispatcher->dispatch_packet(partial_packet, partial_packet->get_info());
		hash_table_remove(home, partial_packet);
	}
}
