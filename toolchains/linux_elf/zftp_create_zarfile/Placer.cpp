#include "Placer.h"
#include "LiteLib.h"
#include "zftp_protocol.h"
#include "CatalogEntry.h"

Placer::Placer(MallocFactory *mf)
{
	current_offset = 0;
	total_size = 0;
	linked_list_init(&placed_list, mf);
}

Placer::~Placer()
{
	lite_assert(placed_list.count==0);
}

uint32_t Placer::gap_size()
{
	return ZFTP_BLOCK_SIZE - (current_offset & (ZFTP_BLOCK_SIZE-1));
}

void Placer::place(Emittable *e)
{
	e->set_location(current_offset);
	linked_list_insert_tail(&placed_list, e);
	current_offset += e->get_size();
}

void Placer::emit(const char *out_path)
{
	FILE *ofp = fopen(out_path, "wb");
	lite_assert(ofp!=NULL);
	uint32_t last_offset = 0;
	while (placed_list.count>0)
	{
		Emittable *e = (Emittable*) linked_list_remove_head(&placed_list);
		e->emit(ofp);

		uint32_t new_offset = ftell(ofp);
		lite_assert(e->get_size() == new_offset - last_offset);
		last_offset = new_offset;

		delete e;
	}
	lite_assert(ftell(ofp)==(int)current_offset);

	if (true)	// round files up to block boundary
	{
		uint32_t rounded_size = ((current_offset-1) & ~(ZFTP_BLOCK_SIZE-1)) + ZFTP_BLOCK_SIZE;
		uint32_t padding_size = rounded_size - current_offset;
		uint8_t zero = 0;
		for (uint32_t i=0; i<padding_size; i++)
		{
			int rc = fwrite(&zero, 1, 1, ofp);
			lite_assert(rc==1);
		}
		current_offset += padding_size;
		lite_assert((current_offset & (ZFTP_BLOCK_SIZE-1)) == 0);
	}
	lite_assert(ftell(ofp)==(int)current_offset);

	total_size = current_offset;

	fclose(ofp);
}
