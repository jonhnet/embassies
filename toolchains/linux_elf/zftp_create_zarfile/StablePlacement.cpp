#include "StablePlacement.h"
#include "HashWriter.h"

StablePlacement::StablePlacement(uint32_t zftp_block_size)
	: zftp_block_size(zftp_block_size)
{
}

void StablePlacement::setup(Placer* placer, ChunkTree* chunk_tree)
{
	uint32_t total_mass = sum_chunk_sizes(chunk_tree);

	canvas_size = total_mass;
	canvas_size += 6*(total_mass/100); // generosity factor to reduce brittleness
	upper_log_canvas_size = 1 << ceillg2(canvas_size-1);

	lite_assert(canvas_size <= upper_log_canvas_size);
	lite_assert(upper_log_canvas_size < 2*canvas_size);

	placer->add_space(canvas_size);
}

uint32_t StablePlacement::sum_chunk_sizes(ChunkTree* chunk_tree)
{
	uint32_t total = 0;
	ChunkBySize *cebs = chunk_tree->findMax();
	while (cebs!=NULL)
	{
		total += cebs->get_chunk_entry()->get_size();
		cebs = chunk_tree->findFirstLessThan(cebs);
	}
	return total;
}

DataRange StablePlacement::preferred_placement(ChunkEntry* chunk, uint32_t seed)
{
	HashWriter hw;
	chunk->emit(&hw);
	hw.write((uint8_t*) &seed, sizeof(seed));
	uint32_t raw_hash = hw.get_hash();
	uint32_t block_aligned_hash = raw_hash & ~(zftp_block_size-1);
		// no point in aligning any finer than block (eg page), since
		// the point is to reduce the number of blocks touched.

	uint32_t mask = upper_log_canvas_size;
	while (mask > 1)
	{
		uint32_t proposed_start = block_aligned_hash & (mask-1);
		if (proposed_start + chunk->get_size() > canvas_size)
		{
			// oh, this placement would slop over the end of the canvas.
			// try another using a smaller mask.
			// Since masks are power-2 sizes, other similar manifests
			// will try the same masks in the same order.
			mask = mask >> 1;
			continue;
		}
		return DataRange(proposed_start, proposed_start + chunk->get_size());
	}
	lite_assert(false);	// somehow the canvas is so small that we can't fit a single file inside it anywhere, even when it's empty?
	return DataRange(0,0);	// satisfy compiler
}

bool StablePlacement::place_one_chunk_preferentially(
	Placer* placer,
	ChunkEntry* entry,
	uint32_t seed)
{
	DataRange location = preferred_placement(entry, seed);
	lite_assert(location.size() == entry->get_size());
	bool placed = placer->place(entry, location.start());
	
#if 0
	fprintf(stderr, "%s for %s : %x at %08x\n",
		placed ? "PLACED" : "failed",
		entry->get_url(),
		entry->get_chdr()->z_data_off,
		location.start());
#endif

	return placed;
}

ChunkTree* StablePlacement::place_chunks_preferentially(
	Placer* placer,
	ChunkTree* chunk_tree)
{
	ChunkTree* scrap_tree = new ChunkTree(&sf);
	// Place every chunk that will fit into its preferred location.
	while (true)
	{
		ChunkBySize *cebs = chunk_tree->findMax();
		if (cebs==NULL)
		{
			break;
		}
		ChunkEntry *next = cebs->get_chunk_entry();
		chunk_tree->remove(cebs);

		bool placed;
		for (uint32_t seed=0; seed<MAX_SEED; seed+=1)
		{
			placed = place_one_chunk_preferentially(placer, next, seed);
			if (placed)
			{
				break;
			}
		}

		if (placed)
		{
			// the ChunkEntry* next belongs to the placer now; we
			// delete the tree wrapper it came in.
			delete cebs;
		}
		else
		{
			// Couldn't put block in its preferred location. Put it on
			// the deferred tree.
			scrap_tree->insert(cebs);
		}
	}
	return scrap_tree;
}

void StablePlacement::place_chunks_arbitrarily(
	Placer* placer,
	ChunkTree* chunk_tree)
{
	while (true)
	{
		ChunkBySize *cebs = chunk_tree->findMax();
		if (cebs==NULL)
		{
			break;
		}
		chunk_tree->remove(cebs);
		ChunkEntry *next = cebs->get_chunk_entry();
		delete cebs;

		placer->place(next);
	}
	lite_assert(chunk_tree->size() == 0);
}

void StablePlacement::place_chunks(Placer* placer, ChunkTree* chunk_tree)
{
	setup(placer, chunk_tree);

	int total_chunks = chunk_tree->size();
	ChunkTree* scrap_tree =
		place_chunks_preferentially(placer, chunk_tree);
	lite_assert(chunk_tree->size() == 0);

	fprintf(stderr, "%d/%d chunks placed arbitrarily\n",
		scrap_tree->size(), total_chunks);

	// By now, hopefully there's not much left.
	place_chunks_arbitrarily(placer, scrap_tree);
}

