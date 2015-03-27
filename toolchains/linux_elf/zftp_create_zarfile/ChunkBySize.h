#pragma once

#include "avl2.h"
#include "ChunkEntry.h"

class ChunkBySize {
public:
	ChunkEntry *chunk;
	uint32_t key_val;

public:
	int cmp(ChunkBySize *other);

public:
	ChunkBySize(ChunkEntry *chunk) : chunk(chunk) {}
	ChunkBySize(uint32_t key_val) : chunk(NULL), key_val(key_val) {}
	ChunkBySize getKey() { return *this; }
	ChunkEntry *get_chunk_entry() { return chunk; }

	inline bool is_key() { return chunk==NULL; }
	uint32_t get_size() { return is_key() ? key_val : chunk->get_size(); }

	bool operator<(ChunkBySize b)
		{ return cmp(&b) <0; }
	bool operator>(ChunkBySize b)
		{ return cmp(&b) >0; }
	bool operator==(ChunkBySize b)
		{ return cmp(&b) ==0; }
	bool operator<=(ChunkBySize b)
		{ return cmp(&b) <=0; }
	bool operator>=(ChunkBySize b)
		{ return cmp(&b) >=0; }
};

typedef AVLTree<ChunkBySize,ChunkBySize> ChunkTree;
