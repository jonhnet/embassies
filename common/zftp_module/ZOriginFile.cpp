#include <assert.h>

#include "xax_network_utils.h"
#include "zftp_util.h"
#include "zlc_util.h"
#include "tree_math.h"
#include "ZCache.h"

#include "ZOriginFile.h"

// origin constructor
ZOriginFile::ZOriginFile(ZCache *zcache, uint32_t file_len, uint32_t flags)
	: ZCachedFile(zcache->GetZLCEmit())
{
	hash_t dummy_hash;
	lite_memset(&dummy_hash, 6, sizeof(dummy_hash));
	_ctor_init(zcache, &dummy_hash);

	metadata.file_len_network_order = z_htong(file_len,
		sizeof(metadata.file_len_network_order));
	metadata.flags = z_htong(flags,
		sizeof(metadata.flags));
	_configure_from_metadata();

	_fill_in_sibling_hashes();
}

void ZOriginFile::_fill_in_sibling_hashes()
{
	// Because the block data doesn't (in general) fill a whole tree,
	// we'll need to supply some dummy sibling hashes to make the tree
	// complete. This'll make the validate step work smoothly.
	if (num_blocks==0)
	{
		tree[0].state = v_validated;
		tree[0].hash = *hash_of_block_of_zeros();
	}
	else
	{
		for (int level=1; level<=tree_depth; level++)
		{
			int lastcol = (num_blocks-1) >> (tree_depth-level);
			uint32_t lastloc = _tree_index(level, lastcol, tree_size);
			uint32_t sibling = _sibling_location(lastloc);
			uint32_t margin_start;

			if (sibling > lastloc)
			{
				ZLC_CHATTY(ze, "_fill_in_sibling_hashes: level %d, lastloc %d, sib %d gets validated\n",, level, lastloc, sibling);
				// There's a stranded sibling to the right of this one.
				// Give it a value.
				tree[sibling].state = v_validated;
				tree[sibling].hash = *hash_of_block_of_zeros();
				margin_start = sibling+1;
			}
			else
			{
				// no free sibling, but we still need v_margin markers
				// to keep from worrying about the unused locs.
				margin_start = lastloc+1;
			}

			uint32_t end_row =
				_tree_index(level, (1<<level)-1, tree_size)+1;
			for (uint32_t margin=margin_start; margin < end_row; margin++)
			{
				// This is a node that should never end up
				// in the calculation, so the fact that it's 'unknown'
				// is okay. Label it a dead node so it doesn't trip
				// a sanity assert later.
				tree[margin].state = v_margin;
				ZLC_CHATTY(ze, "  _fish: margin loc %d\n",, margin);
			}
		}
	}
}

void ZOriginFile::origin_load_block(
	uint32_t block_num, uint8_t *in_data, uint32_t valid_data_len)
{
	ZLC_CHATTY(ze, "origin_load_block(%d)\n",, block_num);
	_load_block(block_num, in_data, valid_data_len);
}

void ZOriginFile::origin_finish_load()
{
	// Once we've fed in all the blocks, _mark_node_tentative should have
	// walked up the tree filling in tentative values all the way.
	// (for 1-block files, root state will already be v_validated.)
	assert(tree[0].state!=v_unknown);

	// Now we have the root hash, so we can compute file_hash. Yay.
	file_hash = _compute_file_hash(&metadata, &tree[0].hash);

	// Mark the tree all validated
	if (num_blocks>0)
	{
		uint32_t last_location = _tree_index(tree_depth, num_blocks-1, tree_size);
		for (uint32_t location=0; location<=last_location; location++)
		{
			assert(tree[location].state != v_unknown);
			tree[location].state = v_validated;
			assert(tree[location].location() == location);
		}
	}
	else
	{
		assert(tree_size==1);
		assert(tree[0].state == v_validated);
		assert(tree[0].location() == 0);
	}

	// Store this hash tree persistently
	ZCachedFile *zcf_live = zcache->lookup_hash(&file_hash);
	_install_into(zcf_live);
}

void ZOriginFile::_track_parent_of_tentative_nodes(ValidatedMerkleRecord *parent)
{
	// origin files don't use the tentative-nodes queue.
}

void ZOriginFile::_assert_root_validated()
{
	// origin files grow from the bottom up, so it's just fine
	// that the root stays tentative 'til we're done.
}

void ZOriginFile::_mark_node_on_block_load(uint32_t location)
{
	if (location==0)
	{
		tree[0].state = v_validated;
	}
	else
	{
		_mark_node_tentative(location);
	}
}

