#include "GreedyPlacement.h"

GreedyPlacement::GreedyPlacement(uint32_t zftp_block_size, bool pack_small_files)
	: zftp_block_size(zftp_block_size),
	  pack_small_files(pack_small_files)
{
}

void GreedyPlacement::place_chunks(
	Placer* placer,
	ChunkTree* chunk_tree)
{
	while (true)
	{
		// try to fill up gaps at the end of a block with small files.
		while (pack_small_files)
		{
			ChunkBySize key(placer->gap_size());
			ChunkBySize *cebs =
				chunk_tree->findFirstLessThanOrEqualTo(&key);
			if (cebs==NULL)
			{
				break;
			}
			ChunkEntry *next_small = cebs->get_chunk_entry();
			chunk_tree->remove(next_small);
#if DEBUG_SCHEDULER
			fprintf(stderr, "I had a gap 0x%08x; best fit was 0x%08x: %s\n",
				placer->gap_size(),
				next_small->get_size(),
				next_small->get_url());
#endif // DEBUG_SCHEDULER
// debug hooks
//			ChunkBySize *d_cebs = chunk_tree->findFirstLessThanOrEqualTo(&key);
//			int c = cebs->cmp(d_cebs);
//			(void) c;
			placer->place(next_small);
		}
		// if there are no more files small enough to fit in the remaining gap,
		// and there are still big files to come, add padding.
		ChunkBySize *cebs = chunk_tree->findMax();
		if (cebs==NULL)
		{
			// oh, we're done.
			break;
		}
		ChunkEntry *next_big = cebs->get_chunk_entry();
		chunk_tree->remove(next_big);
#if DEBUG_SCHEDULER
		fprintf(stderr, "Gap too small 0x%08x; found biggest 0x%08x: %s\n",
			placer->gap_size(),
			next_big->get_size(),
			next_big->get_url());
#endif // DEBUG_SCHEDULER
		if (placer->gap_size() > 0 &&
			(!pack_small_files || next_big->get_size()>zftp_block_size))
		{
			placer->pad(placer->gap_size());
		}
		placer->place(next_big);
	}
}
