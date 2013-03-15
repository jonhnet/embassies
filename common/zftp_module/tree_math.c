#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif // _WIN32
#include "LiteLib.h"
#include "zftp_protocol.h"
#include "tree_math.h"

uint32_t _tree_index(int level, int col, uint32_t tree_size)
{
	uint32_t index = compute_tree_location(level, col);
	lite_assert(index < tree_size);
	return index;
}

void _tree_unindex(uint32_t location, int *level_out, int *col_out)
{
	int level = lite_log2(location+1)-1;
	int col = (location+1) & ((1<<level)-1);
	lite_assert(col>=0);
	*level_out = level;
	*col_out = col;
}

uint32_t _left_child(uint32_t location)
{
	return ((location+1)<<1)-1;
}

uint32_t _right_child(uint32_t location)
{
	return _left_child(location)+1;
}

uint32_t _sibling_location(uint32_t location)
{
	lite_assert(location!=0);
	uint32_t shifted = location + 1;
	shifted = shifted ^ 1;
	return shifted - 1;
}

uint32_t _parent_location(uint32_t location)
{
	uint32_t shifted_loc = location+1;
	uint32_t shifted_parent = shifted_loc >> 1;
	uint32_t parent = shifted_parent - 1;
	return parent;
}

const hash_t *hash_of_block_of_zeros()
{
	static hash_t hash_of_block_of_zeros_val;
	static bool initted = false;
	if (!initted)
	{
		uint8_t *zeros = (uint8_t*) alloca(ZFTP_BLOCK_SIZE);
		lite_memset(zeros, 0, ZFTP_BLOCK_SIZE);
		zhash(zeros, sizeof(ZFTP_BLOCK_SIZE), &hash_of_block_of_zeros_val);
		initted = true;
	}
	return &hash_of_block_of_zeros_val;
}
