#include <malloc.h>
#include <string.h>

#include "ChunkTable.h"

ChunkTable::ChunkTable(MallocFactory *mf)
{
	total_size = 0;
	chunk_index = 0;
	linked_list_init(&chunks, mf);
}

ChunkTable::~ChunkTable()
{
	while (chunks.count>0)
	{
		ZF_Chdr *chunk = (ZF_Chdr*) linked_list_remove_head(&chunks);
		// NB this does *not* own the chunk; we gave it to the caller
		// of allocate(), who handed it off to the Placer, who will
		// be taking care of deleting it.
		// delete chunk;
		chunk = NULL;
	}
}

uint32_t ChunkTable::get_size()
{
	return total_size;
}

uint32_t ChunkTable::allocate(ChunkEntry *chunk)
{
//	ce->assign_string_info(total_size, str_len); okay weird -- how do we know this already?
	
	linked_list_insert_tail(&chunks, chunk);
	total_size += sizeof(ZF_Chdr);
	uint32_t result = chunk_index;
	chunk_index += 1;
	return result;
}

void ChunkTable::emit(FILE *fp)
{
	LinkedListIterator lli;
	for (ll_start(&chunks, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		ChunkEntry *chunk = (ChunkEntry *) ll_read(&lli);
		ZF_Chdr *chdr = chunk->get_chdr();
		int rc = fwrite((uint8_t*) chdr, sizeof(*chdr), 1, fp);
		lite_assert(rc==1);
	}
}

uint32_t ChunkTable::get_count()
{
	return chunks.count;
}
