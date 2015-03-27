#pragma once

#include "PlacementIfc.h"
#include "DataRange.h"
#include "SyncFactory_Pthreads.h"

class StablePlacement : public PlacementIfc
{
private:
	enum { MAX_SEED = 10 };
		// How many times to look for stable hash-inspired placements before
		// punting and going greedy.

	SyncFactory_Pthreads sf;
	uint32_t canvas_size;
	uint32_t upper_log_canvas_size;
	uint32_t zftp_block_size;

	void setup(Placer* placer, ChunkTree* chunk_tree);

	uint32_t sum_chunk_sizes(ChunkTree* chunk_tree);

	DataRange preferred_placement(ChunkEntry* chunk, uint32_t seed);

	bool place_one_chunk_preferentially(
		Placer* placer,
		ChunkEntry* entry,
		uint32_t seed);

	ChunkTree* place_chunks_preferentially(
		Placer* placer,
		ChunkTree* chunk_tree);

	ChunkTree* place_chunks_preferentially(
		Placer* placer,
		ChunkTree* chunk_tree,
		uint32_t seed);

	void place_chunks_arbitrarily(
		Placer* placer,
		ChunkTree* chunk_tree);

public:
	StablePlacement(uint32_t zftp_block_size);

	virtual void place_chunks(
		Placer* placer,
		ChunkTree* chunk_tree);
};
