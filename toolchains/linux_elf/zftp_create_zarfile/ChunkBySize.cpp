#include <stdio.h>
#include <stdint.h>
#include "ChunkBySize.h"

int ChunkBySize::cmp(ChunkBySize *other)
{
	int size_diff = get_size() - other->get_size();

	if (size_diff!=0) { return size_diff; }

	// keys are always "bigger" than the objects of the same size,
	// so that when we look to their left, we find the
	// deterministically-biggest chunk (including names and offsets to
	// break ties).
	lite_assert(!(this->is_key() && other->is_key()));
	if (this->is_key()) { return 1; }
	if (other->is_key()) { return -1; }

	// Compare on files and offsets,
	// so that content appears in a deterministic order.
	int name_diff = strcmp(chunk->get_url(), other->chunk->get_url());
	if (name_diff!=0) { return name_diff; }

	// well, sometimes the same file can have several chunks. Deeper!
	int off_diff = (chunk->get_chdr()->z_data_off - other->chunk->get_chdr()->z_data_off);
	if (off_diff!=0) { return off_diff; }

	// Okay, but it might have several copies of the *same* chunk.
	// At this point, disambiguate on address, since it's the same content
	// and order really doesn't matter, but they need to be able to coexist
	// in a duplicate-free AVL tree.
 	return ((uint32_t) chunk) - ((uint32_t) other->chunk);
}

