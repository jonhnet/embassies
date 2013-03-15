#error dead code
#include <malloc.h>
#include <stdlib.h>

#include "LiteLib.h"
#include "CatalogEntry.h"
#include "ZIndex.h"
#include "StringIndex.h"

ZIndex::ZIndex(int num_entries, StringIndex *string_index)
	: num_entries(num_entries),
	  string_index(string_index)
{
	ce_array = (CatalogEntry **) malloc(sizeof(CatalogEntry *)*num_entries);
}

ZIndex::~ZIndex()
{
}

void ZIndex::assign(int slot/*, uint32_t data_offset*/, CatalogEntry *ce)
{
	lite_assert(slot<num_entries);
	ce_array[slot] = ce;
//	ce->assign_data_offset(data_offset);
	string_index->allocate(ce);
}

uint32_t ZIndex::get_size()
{
	return sizeof(ZF_Phdr) * num_entries;
}

void ZIndex::emit(FILE *fp)
{
	// entries must be sorted by url for receiver to be able to binsearch.
	// They're insterted in packing order, though, so we need to sort
	// them now, before we emit them.
	qsort(ce_array, num_entries, sizeof(ce_array[0]), CatalogEntry::cmp_by_url);

	for (int i=0; i<num_entries; i++)
	{
		const ZF_Phdr *phdr = ce_array[i]->peek_phdr();
		int rc = fwrite(phdr, sizeof(*phdr), 1, fp);
		lite_assert(rc==1);
	}
}
