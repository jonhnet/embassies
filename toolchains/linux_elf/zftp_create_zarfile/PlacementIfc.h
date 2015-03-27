#pragma once

#include "Placer.h"
#include "ChunkBySize.h"

class PlacementIfc {
public:
	virtual void place_chunks(
		Placer* placer,
		ChunkTree* chunk_tree) = 0;
};
