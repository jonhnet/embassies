#include <malloc.h>
#include <string.h>

#include "standard_malloc_factory.h"
#include "ChunkEntry.h"
#include "ZFSReader.h"
#include "CatalogEntry.h"
#include "StdioWriter.h"

ChunkEntry::ChunkEntry(uint32_t offset, uint32_t len, ChunkType chunk_type, CatalogEntry *catalog_entry)
{
	_chdr.z_file_off = (uint32_t) -1;
	_chdr.z_data_off = offset;
	_chdr.z_data_len = len;
	_chdr.z_precious = (chunk_type==PRECIOUS);
	this->catalog_entry = catalog_entry;
}

ChunkEntry::~ChunkEntry()
{
}

void ChunkEntry::set_location(uint32_t offset)
{
	Emittable::set_location(offset);
	_chdr.z_file_off = offset;
}

ZF_Chdr* ChunkEntry::get_chdr()
{
	return &_chdr;
}

uint32_t ChunkEntry::get_size()
{
	return (int) _chdr.z_data_len;
}

const char* ChunkEntry::get_type()
{
	return "ChunkEntry";
}

void ChunkEntry::emit(FILE *fp)
{
	StdioWriter writer(fp);
	emit(&writer);
}

void ChunkEntry::emit(WriterIfc* writer)
{
	ZF_Chdr* chdr = get_chdr();
	ZFSReader *zf = catalog_entry->get_reader();

	// scheduler can ask us to insert more bytes than are in the file,
	// when we're setting up for a fast_mmap operation. In that case,
	// we want to simulate mmap() semantics, wherein the rest of the
	// requested range is zeros.
	uint32_t zero_fill_len = 0;
	uint32_t copy_len = chdr->z_data_len;

	uint32_t filelen;
	bool rcb = zf->get_filelen(&filelen);
	lite_assert(rcb);

	if (chdr->z_data_off+chdr->z_data_len > filelen)
	{
		lite_assert(!chdr->z_precious);
			// precious reads never need tail zeros, because the
			// client can provide them after the mmap anyway.
		
		zero_fill_len = chdr->z_data_off + chdr->z_data_len - filelen;
		copy_len = filelen - chdr->z_data_off;
	}

	lite_assert(copy_len <= filelen);
	lite_assert(copy_len + zero_fill_len == chdr->z_data_len);

	zf->copy(writer, chdr->z_data_off, copy_len);
	if (zero_fill_len>0)
	{
		uint8_t* zero_buf = (uint8_t*) malloc(zero_fill_len);
		memset(zero_buf, 0, zero_fill_len);
		writer->write(zero_buf, zero_fill_len);
		free(zero_buf);
	}
	delete zf;
}

const char* ChunkEntry::get_url()
{
	return catalog_entry->get_url();
}
