#include "RequestBuilder.h"
#include "math_util.h"
#include "tree_math.h"
#include "zemit.h"


RequestBuilder::RequestBuilder(ZCachedFile *zcf, SyncFactory *sf, DataRange missing_set, uint32_t mtu)
	: zcf(zcf),
	  mtu(mtu),
	  url_hint(NULL),
	  committed_tree_locations(sf),
	  tentative_tree_locations(sf)
{
	_build_request(missing_set);
}

RequestBuilder::~RequestBuilder()
{
	while (true)
	{
		Avl_uint32_t *elt = committed_tree_locations.findMin();
		if (elt==NULL)
		{
			break;
		}
		committed_tree_locations.remove(elt);
	}
}

void RequestBuilder::_build_request(DataRange missing_set)
{
	// NB by hypothesis if we're here, we've got block 0. If we didn't have
	// block zero (and the file metadata), we wouldn't be able to
	// formulate a request for the other blocks we want. It adds an RTT,
	// but this is the incremental-fetch path in any case, so we're already
	// well inside RTT-land.

	lite_assert(tentative_tree_locations.size()==0);
	uint32_t start = missing_set.start();
	request_range = DataRange();

	while (start < missing_set.end() && _available()>0)
	{
		uint32_t blk = start>>ZFTP_LG_BLOCK_SIZE;
		uint32_t end_blk = (blk+1)<<ZFTP_LG_BLOCK_SIZE;

		ZLC_CHATTY(zcf->ze, "  blk %d @ %d; available now %d\n",, blk, start, _available());
		request_hash_ancestors(blk);
		if (_available() < 0)
		{
			// drat -- we went over-budget on hash requests alone.
			// Skip these, and the data they'd ride in on.
			clear_tentative_locations(ABORT);
			break;
		}
		else
		{
			clear_tentative_locations(COMMIT);
		}

		// only ask for data blocks when leaf is known, to avoid
		// (in the warm-cache case) asking for data that will turn out to
		// be redundant once we learn the leaf hash.
		uint32_t leaf_location = _tree_index(zcf->tree_depth, blk, zcf->tree_size);
		ValidatedMerkleRecord *vmr = &zcf->tree[leaf_location];
		bool leaf_known = (vmr->state == v_validated);

		if (leaf_known)
		{
			uint32_t req_end = min(end_blk, missing_set.end());
			DataRange req_this_blk(start, req_end);
			if (((int) req_this_blk.size()) > _available())
			{
				req_this_blk = DataRange(start, start+_available());
			}
			request_range.accumulate(req_this_blk);
		}

		start = end_blk;
	}

	lite_assert(tentative_tree_locations.size()==0);

	optimize_requested_hash_list();
	lite_assert(_available() >= 0);
}

void RequestBuilder::clear_tentative_locations(Disposition disposition)
{
	while (true)
	{
		Avl_uint32_t *elt = tentative_tree_locations.findMin();
		if (elt==NULL)
		{
			break;
		}
		if (disposition == COMMIT)
		{
			committed_tree_locations.insert(elt);
		}
		tentative_tree_locations.remove(elt);
	}
}

void RequestBuilder::optimize_requested_hash_list()
{
	// Now, if we've made requests for data that completes a block,
	// that may buy us a hash we'd planned on requesting. That hash
	// or others may also have ended up making other sibling requests
	// higher in the tree redundant. Detect redundant requests and cull
	// them.
	// TODO unimplemented, because the cost of duplicate hash shipping
	// is pretty low. We're not actually re-COMPUTING the redundant hashes.
	// Plus we'd need to get more special-casey in the receive side code
	// to be able realize that we have a whole block, and we can hash it to
	// establish the appropriate tentative hash.
}

void RequestBuilder::request_block_hash(uint32_t block_num)
{
	uint32_t location = _tree_index(zcf->tree_depth, block_num, zcf->tree_size);
	request_hash(location);
}

bool RequestBuilder::request_hash(uint32_t location)
{
	bool result = false;
	if (committed_tree_locations.lookup(location) == NULL
		&& tentative_tree_locations.lookup(location) == NULL)
	{
		ValidatedMerkleRecord *vmr = &zcf->tree[location];
		if (vmr->state == v_unknown)
		{
			tentative_tree_locations.insert(vmr->get_avl_elt());
			result = true;
		}
	}
	ZLC_CHATTY(zcf->ze, "  request_hash(%d) = %d\n",, location, result);
	return result;
}

void RequestBuilder::request_hash_ancestors(uint32_t block_num)
{
	// compute all the ancestors of the blocks we're sending.
	// these are the hashes that the client will need to validate.
	int level;
	for (level = zcf->tree_depth; level>0; level--)
		// NB no need to go to 0; we have that one already by hypothesis.
	{
		uint32_t leveldiff = zcf->tree_depth - level;
		uint32_t col = block_num >> leveldiff;
		uint32_t location = _tree_index(level, col, zcf->tree_size);

		request_hash(_sibling_location(location));
		bool request_is_novel = request_hash(location);

		if (!request_is_novel)
		{
			// no need to go to root; someone else has already been there.
			break;
		}
	}
}

int RequestBuilder::_available()
{
	int consumed =
		sizeof(ZFTPDataPacket)
		+ sizeof(ZFTPMerkleRecord) *
			(committed_tree_locations.size() + tentative_tree_locations.size())
		+ request_range.size();
	return ((int) mtu) - consumed;
}

OutboundRequest *RequestBuilder::as_outbound_request()
{
	ZLC_CHATTY(zcf->ze, "  requesting data at %d\n",, request_range.start());
	OutboundRequest *obr = new OutboundRequest(
		zcf->mf,
		&zcf->file_hash,
		url_hint,
		committed_tree_locations.size(),
		0,
		request_range);

	uint32_t i;
	Avl_uint32_t *elt;
	for (elt = committed_tree_locations.findMin(), i=0;
		NULL!=elt;
		elt=committed_tree_locations.findFirstGreaterThan(elt), i++)
	{
		obr->set_location(i, elt->getKey());
	}
	return obr;
}

bool RequestBuilder::requested_any_data()
{
	return request_range.size()>0;
}
