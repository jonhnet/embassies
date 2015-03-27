#pragma once

#include "PlacementIfc.h"

class GreedyPlacement : public PlacementIfc {
private:
	uint32_t zftp_block_size;
	bool pack_small_files;

public:
	GreedyPlacement(uint32_t zftp_block_size, bool pack_small_files);

	virtual void place_chunks(
		Placer* placer,
		ChunkTree* chunk_tree);
};
