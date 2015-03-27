#include <malloc.h>
#include <string.h>
#include <sys/stat.h>

#include "zftp_protocol.h"
#include "ZFSReader.h"
#include "standard_malloc_factory.h"
#include "elf_flat_reader.h"
#include "elfobj.h"

#include "Catalog.h"
#include "CatalogEntry.h"
#include "ChunkEntry.h"
#include "Placer.h"
#include "ZHdr.h"
#include "PHdr.h"
#include "Padding.h"
#include "math_util.h"
#include "CatalogElfReaderWrapper.h"
#include "CatalogEntryByUrl.h"
#include "GreedyPlacement.h"
#include "StablePlacement.h"

#define DEBUG_SCHEDULER 1


Catalog::Catalog(ZCZArgs *zargs, SyncFactory *sf)
	: mf(standard_malloc_factory_init()),
	  sf(sf),
	  trace_decoder(NULL),
	  url_tree(sf),
	  chunk_tree(sf),
	  string_table(NULL),
	  chunk_table(NULL),
	  zargs(zargs),
	  file_system_view(mf, sf, zargs->hide_icons_dir),
	  placer(NULL),
	  zhdr(NULL),
	  zftp_lg_block_size(zargs->zftp_lg_block_size),
	  zftp_block_size(1<<zargs->zftp_lg_block_size)
{
	file_system_view.insert_overlay("");
	while (zargs->overlay_list.count > 0)
	{
		const char *overlay_path =
			(const char *) linked_list_remove_head(&zargs->overlay_list);
		file_system_view.insert_overlay(overlay_path);
	}
}

void Catalog::add_entry(CatalogEntry *ce)
{
	if (url_tree.lookup(CatalogEntryByUrl(ce))!=NULL)
	{
		// duplicate
		delete ce;
		return;
	}
	url_tree.insert(new CatalogEntryByUrl(ce));
}

void Catalog::add_record_noent(const char *path)
{
	char* file_url = cons(FILE_SCHEME, path);
	add_entry(new CatalogEntry(file_url, false, 0, ZFTP_METADATA_FLAG_ENOENT, &file_system_view));
}

bool Catalog::add_record_one_url(const char *url)
{
	ZFSReader *zf = file_system_view.open_url(url);
	if (zf==NULL)
	{
		return false;
	}

	bool rc;
	uint32_t len;
	bool is_dir;

	rc = zf->get_filelen(&len);
	lite_assert(rc);

	rc = zf->is_dir(&is_dir);
	lite_assert(rc);

	delete zf;

	uint32_t flags =
		is_dir
		? ZFTP_METADATA_FLAG_ISDIR
		: CatalogEntry::FLAGS_NONE;
	CatalogEntry *ce = new CatalogEntry(url, true, len, flags, &file_system_view);
	add_entry(ce);

	return true;
}

char* Catalog::cons(const char* scheme, const char* path)
{
	int size = strlen(scheme)+strlen(path)+1;
	char* s = (char*) malloc(size);
	strcpy(s, scheme);
	strcat(s, path);
	return s;
}

bool Catalog::add_record_one_path(const char *path)
{
	char* file_url = cons(FILE_SCHEME, path);
	bool rc;

	rc = add_record_one_url(file_url);
	if (!rc)
	{
		free(file_url);
		return false;
	}

	char* stat_url = cons(STAT_SCHEME, path);
	rc = add_record_one_url(stat_url);
	lite_assert(rc);

	free(file_url);
	free(stat_url);
	return true;
}


enum FilterResult { INCLUDE, TOMBSTONE, OMIT };
FilterResult filter_path(const char *path, ZCZArgs *zargs)
{
	if (strncmp(path, "/proc", 5)==0)
	{
		if (zargs->verbose)
		{
			fprintf(stderr, "skipping proc : %s\n", path);
		}
		return OMIT;
	}

	if (!zargs->include_elf_syms)
	{
		ElfFlatReader efr;
		efr_read_file(&efr, path);
			// TODO wasteful; reads whole file to scan for symtab.
			// But, for now, who cares? this is a rarely-executed dev tool

		ElfObj elfobj, *eo=&elfobj;
		bool is_elf = elfobj_scan(eo, &efr.ifc);
		bool unstripped_elf = (is_elf && elfobj_find_section_index_by_name(eo, ".symtab")!=-1);
		if (is_elf)
		{
			elfobj_free(eo);
		}
		if (unstripped_elf)
		{
			if (zargs->verbose)
			{
				fprintf(stderr, "skipping unstripped ELF: %s\n", path);
			}
			return TOMBSTONE;
		}
	}

	return INCLUDE;
}

void Catalog::s_scan_foreach(void* v_this, void* v_tpr)
{
	((Catalog*) v_this)->scan_foreach((TracePathRecord*) v_tpr);
}

void Catalog::scan_foreach(TracePathRecord* tpr)
{
	struct stat statbuf;
	int rc = stat(tpr->path, &statbuf);

	FilterResult result;
	if (rc==0)
	{
		if (S_ISDIR(statbuf.st_mode))
		{
			result = INCLUDE;
		}
		else
		{
			result = filter_path(tpr->path, zargs);
		}
	}

	bool catalog_successful = false;
	if (result != OMIT)
	{
		catalog_successful = add_record_one_path(tpr->path);
		if (!catalog_successful)
		{
			add_record_noent(tpr->path);
		}
	}
}

void Catalog::scan(const char *trace)
{
	trace_decoder = new TraceDecoder(trace, mf, sf);
	trace_decoder->visit_trace_path_records(s_scan_foreach, this);
}

char* Catalog::compute_mmap_filename(const char* log_paths)
{
	char* dest = (char*) malloc(strlen(log_paths)+10);

	const char* paths_pos = strstr(log_paths, "paths");
	lite_assert(paths_pos!=NULL);
	int prefix_len = (int) (paths_pos - log_paths);
	memcpy(dest, log_paths, prefix_len);
	dest[prefix_len] = '\0';
	strcat(dest, "mmap");
	strcat(dest, &paths_pos[5]);
	return dest;
}

void Catalog::emit(const char *out_path)
{
	PlacementIfc* placement_algorithm;
	if (zargs->placement_algorithm_enum == GREEDY_PLACEMENT)
	{
		placement_algorithm = new GreedyPlacement(zftp_block_size, zargs->pack_small_files);
	}
	else if (zargs->placement_algorithm_enum == STABLE_PLACEMENT)
	{
		placement_algorithm = new StablePlacement(zftp_block_size);
	}
	else
	{
		lite_assert(false);
	}

	// The placer keeps track of file offsets we've already nailed down.
	placer = new Placer(mf, zftp_block_size);

	zhdr = new ZHdr(zftp_lg_block_size);
	placer->place(zhdr);

	string_table = new StringTable(mf);
	chunk_table = new ChunkTable(mf);

	// Enumerate entries into chunk tree without placing them;
	// count up space for string, chunk (CHdr) tables
	// (Strings and CHdrs could be allocated alongside all the actual
	// data chunks, but that might scatter them throughout the file,
	// making it expensive to walk the index in a partially-loaded zarfile.)
	enumerate_entries();

	// NB we could move the chunk table to the end of the file, if we wanted,
	// to allow for a placement algorithm that changed the number of chunks.
	// But keeping it at the front means that it's in the same block as the
	// ZHdr, reducing the number of blocks that are *always* changed when
	// the input changes. (We could also just leave a little headroom in
	// the chunktable, or repeat the placement computation until we reach a
	// fixpoint.)
	placer->place(string_table);
	zhdr->connect_string_table(string_table);
	placer->place(chunk_table);
	zhdr->connect_chunk_table(chunk_table);
	fprintf(stderr, "string_table %d bytes\n", string_table->get_size());
	fprintf(stderr, " chunk_table %d bytes\n", chunk_table->get_size());

	// Now walk through it, making placement decisions to pack big
	// chunks nicely.
	placement_algorithm->place_chunks(placer, &chunk_tree);

	zhdr->set_dbg_zarfile_len(placer->get_offset());

	// Now pass over the list of placed Emittables and actually squirt them out.
	placer->emit(out_path);

	placer->dbg_report();

	delete placer;
}

class ChunkCounter {
public:
	bool fresh;
	int first_chunk_idx;
	int count;
	ChunkCounter() : fresh(true), first_chunk_idx(-1), count(0) {}
};

void Catalog::add_chunk(uint32_t offset, uint32_t len, ChunkEntry::ChunkType chunk_type, CatalogEntry *ce, ChunkCounter *cc)
{
	ChunkEntry *chunk = new ChunkEntry(offset, len, chunk_type, ce);
	int idx = chunk_table->allocate(chunk);
	chunk_tree.insert(new ChunkBySize(chunk));

	if (cc->fresh)
	{
		cc->first_chunk_idx = idx;
		cc->fresh = false;
	}
	cc->count += 1;
}

const char *Catalog::path_from_url(const char *url)
{
	lite_assert(strncmp(url, FILE_SCHEME, 5)==0);
	return &url[5];
}

void Catalog::enumerate_entries()
{
	uint32_t phdr_count = 0;

	while (true)
	{
		CatalogEntryByUrl* cebu = url_tree.findMin();
		if (cebu==NULL) { break; }
		url_tree.remove(cebu);

		CatalogEntry* ce = cebu->get_catalog_entry();

		PHdr* phdr = ce->take_phdr();
		placer->place(phdr);
		if (phdr_count == 0)
		{
			zhdr->connect_index(phdr);
		}
		const char *url = ce->get_url();
		uint32_t string_index = string_table->allocate(url);
		phdr->connect_string(string_index, strlen(url));
		phdr_count += 1;

		ChunkCounter cc;

		if (lite_starts_with(STAT_SCHEME, url))
		{
			// we synthesized this record; it's not in the trace_decoder.
			add_chunk(0, phdr->get_file_len(), ChunkEntry::PRECIOUS, ce, &cc);
		}
		else if (phdr->is_dir())
		{
			// We don't record the range of reads for dirs in the trace client.
			// (Not sure why.) But in any case, it seems like we'll always
			// want the whole dir anyway, since dirs aren't in any particular
			// order.
			add_chunk(0, phdr->get_file_len(), ChunkEntry::PRECIOUS, ce, &cc);
		}
		else if (phdr->is_enoent())
		{
			// ENOENT records might have tprs anyway, when Jeremy's
			// synthesizing them. Just skip them, to avoid getting
			// confused between the ENOENT phdr with length 0
			// and the non-empty tpr precious chunk.
		}
		else
		{
			const char* path = path_from_url(url);
			TracePathRecord* tpr = trace_decoder->lookup(path);
#define DEBUG_CHUNKS 0
#if DEBUG_CHUNKS
			fprintf(stderr, "Path: %s\n", path);
#endif // DEBUG_CHUNKS

			// Fast chunks
			LinkedListIterator lli;
			for (ll_start(&tpr->fast_chunks, &lli);
				ll_has_more(&lli);
				ll_advance(&lli))
			{
				FastChunk* fast_chunk = (FastChunk*) ll_read(&lli);
#if DEBUG_CHUNKS
				fprintf(stderr, "  fast %08x\n", fast_chunk->size);
#endif // DEBUG_CHUNKS
				add_chunk(fast_chunk->range.start, fast_chunk->range.size(), ChunkEntry::FAST, ce, &cc);
			}

			// Precious chunks
			Range cursor;
			bool found;
			for (found = tpr->precious_chunks->first_range(&cursor);
				found;
				found = tpr->precious_chunks->next_range(cursor, &cursor))
			{
#if DEBUG_CHUNKS
				fprintf(stderr, "  precious %08x -- %08x\n", cursor.start, cursor.end);
#endif // DEBUG_CHUNKS
				// don't (try to) store precious data past EOF; those come
				// from mmap rounding up to page boundaries; the zeros
				// are supplied by the client, not us.
				Range trimmed(cursor.start,
					min(cursor.end, phdr->get_file_len()));
				lite_assert(trimmed.size()>0);
				add_chunk(trimmed.start, trimmed.size(), ChunkEntry::PRECIOUS, ce, &cc);
			}
		}
		lite_assert(!(phdr->get_file_len()==0 && cc.count>0)); // chunks without any file contents?!?
		phdr->connect_chunks(cc.first_chunk_idx, cc.count);
	}
	zhdr->set_index_count(phdr_count);
}

void Catalog::place_chunks()
{
}
