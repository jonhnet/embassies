#include <assert.h>
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
#include "MmapDecoder.h"
#include "CatalogElfReaderWrapper.h"

class CatalogEntryByUrl {
private:
	CatalogEntry *ce;

public:
	CatalogEntryByUrl(CatalogEntry *ce) : ce(ce) {}
	CatalogEntryByUrl getKey() { return *this; }
	CatalogEntry* get_catalog_entry() { return ce; }

	bool operator<(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())<0; }
	bool operator>(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())>0; }
	bool operator==(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())==0; }
	bool operator<=(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())<=0; }
	bool operator>=(CatalogEntryByUrl b)
		{ return strcmp(ce->get_url(), b.ce->get_url())>=0; }
};

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

//////////////////////////////////////////////////////////////////////////////

const char *path_from_url(const char *url)
{
	assert(strncmp(url, FILE_SCHEME, 5)==0);
	return &url[5];
}

//////////////////////////////////////////////////////////////////////////////

Catalog::Catalog(ZArgs *zargs, SyncFactory *sf)
	: mf(standard_malloc_factory_init()),
	  mmap_decoder(NULL),
	  url_tree(sf),
	  chunk_tree(sf),
	  string_table(NULL),
	  chunk_table(NULL),
	  zargs(zargs),
	  file_system_view(mf, sf),
	  placer(NULL),
	  zhdr(NULL),
	  dbg_total_padding(0)
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

void Catalog::add_record_noent(const char *url)
{
	add_entry(new CatalogEntry(url, false, 0, ZFTP_METADATA_FLAG_ENOENT, &file_system_view));
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
	assert(rc);

	rc = zf->is_dir(&is_dir);
	assert(rc);

	delete zf;

	uint32_t flags =
		is_dir
		? ZFTP_METADATA_FLAG_ISDIR
		: CatalogEntry::FLAGS_NONE;
	CatalogEntry *ce = new CatalogEntry(url, true, len, flags, &file_system_view);
	add_entry(ce);

	return true;
}

bool Catalog::add_record_one_file(const char *url)
{
	bool rc;
	rc = add_record_one_url(url);
	if (!rc)
	{
		return false;
	}

	const char *path = path_from_url(url);

	int urlsize = strlen(STAT_SCHEME)+strlen(path)+1;
	char *staturl = (char*) malloc(urlsize);
	snprintf(staturl, urlsize, "%s%s", STAT_SCHEME, path);
	rc = add_record_one_url(staturl);
	assert(rc);
	free(staturl);
	return true;
}


enum FilterResult { INCLUDE, TOMBSTONE, OMIT };
FilterResult filter_path(const char *path, ZArgs *zargs)
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

bool str_ends_with(const char *s, const char *sub)
{
	int slen = strlen(s);
	int sublen = strlen(sub);

	if (slen < sublen)
	{
		return false;
	}

	return strcmp(&s[slen-sublen], sub)==0;
}

void Catalog::scan(const char *log_paths, const char *dep_file_name, const char *dep_target_name)
{
	FILE *dep_file_fp = NULL;
	if (dep_file_name!=NULL)
	{
		dep_file_fp = fopen(dep_file_name, "w");
		fprintf(dep_file_fp, "%s: \\\n", dep_target_name);
	}

	FILE *fp = fopen(log_paths, "r");
	assert(fp!=NULL);
	char buf[800];
	int rc;
	while (true)
	{
		char *line = fgets(buf, sizeof(buf), fp);
		if (line==NULL)
		{
			break;
		}
		assert(line[strlen(line)-1]=='\n');
		line[strlen(line)-1]='\0';

		const char *url = line;
		if (lite_starts_with(STAT_SCHEME, url))
		{
			continue;
		}

		const char *path = path_from_url(url);

		if (str_ends_with(url, ".zarfile"))
		{
			// avoid getting all recursey
//			fprintf(stderr, "Skipping zarfile %s\n", url);
			continue;
		}

		if (dep_file_name!=NULL)
		{
			fprintf(dep_file_fp, "	%s \\\n", path);
		}

		struct stat statbuf;
		rc = stat(path, &statbuf);
	
		FilterResult result;
		if (rc==0)
		{
			if (S_ISDIR(statbuf.st_mode))
			{
				result = INCLUDE;
			}
			else
			{
				result = filter_path(path, zargs);
			}
		}

		bool catalog_successful = false;
		if (result != OMIT)
		{
			catalog_successful = add_record_one_file(url);
			if (!catalog_successful)
			{
				add_record_noent(url);
			}
		}
	}

	if (dep_file_name!=NULL)
	{
		fprintf(dep_file_fp, "\n");
		fclose(dep_file_fp);
	}

	char* mmap_file = compute_mmap_filename(log_paths);
	mmap_decoder = new MmapDecoder(mmap_file);
	free(mmap_file);
//	mmap_decoder->dbg_dump();
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
	// The placer keeps track of file offsets we've already nailed down.
	placer = new Placer(mf);

	zhdr = new ZHdr();
	placer->place(zhdr);

	string_table = new StringTable(mf);
	chunk_table = new ChunkTable(mf);

	// Enumerate entries into chunk tree without placing them;
	// count up space for string, chunk (CHdr) tables
	// (Strings and CHdrs could be allocated alongside all the actual
	// data chunks, but that might scatter them throughout the file,
	// making it expensive to walk the index in a partially-loaded zarfile.)
	enumerate_entries();

	placer->place(string_table);
	zhdr->connect_string_table(string_table);
	placer->place(chunk_table);
	zhdr->connect_chunk_table(chunk_table);

	// Now walk through it, making placement decisions to pack big
	// chunks nicely.
	place_chunks();

	zhdr->set_dbg_zarfile_len(placer->get_offset());

	// Now pass over the list of placed Emittables and actually squirt them out.
	placer->emit(out_path);

	fprintf(stderr, "Total padding used: %.1fkB (%.3f%%)\n",
		dbg_total_padding*1.0/(1<<10),
		dbg_total_padding*100.0/placer->get_total_size());
}

class ChunkCounter {
public:
	bool fresh;
	int first_chunk_idx;
	int count;
	ChunkCounter() : fresh(true), first_chunk_idx(-1), count(0) {}
};

void Catalog::add_chunk(uint32_t offset, uint32_t len, bool precious, CatalogEntry *ce, ChunkCounter *cc)
{
	ChunkEntry *chunk = new ChunkEntry(offset, len, precious, ce);
	int idx = chunk_table->allocate(chunk);
	chunk_tree.insert(new ChunkBySize(chunk));

	if (cc->fresh)
	{
		cc->first_chunk_idx = idx;
		cc->fresh = false;
	}
	cc->count += 1;
}

void Catalog::lookup_elf_ranges(ZFSReader* zf, LinkedList* out_ranges)
{
	linked_list_init(out_ranges, standard_malloc_factory_init());

	CatalogElfReaderWrapper cerw(zf);
	if (cerw.eo() == NULL) { return; }

	uint32_t start, len;

	// section header table
	start = cerw.eo()->ehdr->e_shoff;
	len = cerw.eo()->ehdr->e_shnum * cerw.eo()->ehdr->e_shentsize;
	linked_list_insert_tail(out_ranges, new MDRange(start, start+len));

	// section header strings
	start = cerw.eo()->shdr[cerw.eo()->ehdr->e_shstrndx].sh_offset;
	len = cerw.eo()->shdr[cerw.eo()->ehdr->e_shstrndx].sh_size;
	linked_list_insert_tail(out_ranges, new MDRange(start, start+len));

	// strtab. (Maybe overkill; could tune ElfObj to not bother
	// reading that stuff until required?)
	int strtab_section_idx =
		elfobj_find_section_index_by_name(cerw.eo(), ".strtab");
	if (strtab_section_idx != (int)-1)
	{
		ElfWS_Shdr *strtab_shdr = &cerw.eo()->shdr[strtab_section_idx];
			
		start = strtab_shdr->sh_offset;
		len = strtab_shdr->sh_size;
		linked_list_insert_tail(out_ranges, new MDRange(start, start+len));
	}
}

void Catalog::insert_elf_chunks(ZFSReader* zf, CatalogEntry* ce, ChunkCounter* cc)
{
	CatalogElfReaderWrapper cerw(zf);
	if (cerw.eo() == NULL) { return; }

	int phdr_count;
	ElfWS_Phdr **phdrs = elfobj_get_phdrs(cerw.eo(), PT_LOAD, &phdr_count);
	uint32_t max_mem_reserved = 0;
	for (int phdr_idx=0; phdr_idx<phdr_count; phdr_idx++)
	{
		uint32_t mem_start = phdrs[phdr_idx]->p_vaddr - phdrs[0]->p_vaddr;
		uint32_t mem_end = mem_start + phdrs[phdr_idx]->p_memsz;
		max_mem_reserved = max(max_mem_reserved, mem_end);
	}
	uint32_t file_len = cerw.get_filelen();
	uint32_t last_file_byte = min(max_mem_reserved, file_len);
	uint32_t end_text_segment = phdrs[0]->p_filesz;
	uint32_t first_byte = end_text_segment & ~0xfff;	// round to page start
	add_chunk(first_byte, last_file_byte - first_byte, true, ce, cc);

	free(phdrs);
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
		uint32_t len = phdr->get_file_len();
		if (len>0)
		{
			ZFSReader *zf = file_system_view.open_url(url);

			MmapBehavior* behavior = NULL;
			if (lite_starts_with(FILE_SCHEME, url))
			{
				const char *path = path_from_url(url);
				behavior = mmap_decoder->query_file(path, false);
			}
			if (behavior!=NULL)
			{
				lite_assert(zf!=NULL);	// but it was mmap()ed in the log! Huh?
				bool rc;
				uint32_t filelen;
				rc = zf->get_filelen(&filelen);
				lite_assert(rc);

				uint32_t behavior_size = behavior->aligned_mapping_size;

				LinkedList elf_ranges;
				lookup_elf_ranges(zf, &elf_ranges);
				zf = NULL;	// lookup_elf_ranges consumes zf
				while (true)
				{
					MDRange* range = (MDRange*)
						linked_list_remove_head(&elf_ranges);
					if (range==NULL) { break; }
					behavior->insert_range(range);
					delete range;
				}

#define INCLUDE_ENTIRE_MMAPPED_FILES 0
#if INCLUDE_ENTIRE_MMAPPED_FILES
				// stretch fast_mmap size to entire file size, to ensure
				// section headers can be read by elfobj_scan.
				behavior_size = max(behavior_size, filelen);
				// TODO this is a source of waste; it'll needlessly slurp
				// in symbols in unstripped files, since they come before
				// the section headers. (viz. midori.zguest).
				// Rats! That cost me 120-74M = 46M! That's not gonna work.
#endif

				// and now round up modified len to a page, since fast_mmap
				// needs to find zeros there.
				behavior_size = ((behavior_size-1)&(~0xfff))+0x1000;

				for (uint32_t copy=0; copy<behavior->aligned_mapping_copies; copy++)
				{
					add_chunk(0, behavior_size, false, ce, &cc);
				}

				LinkedListIterator lli;
				for (ll_start(&behavior->precious_ranges, &lli);
					ll_has_more(&lli);
					ll_advance(&lli))
				{
					MDRange* range = (MDRange*) ll_read(&lli);
					// precious ranges are always memcpy()d, and hence
					// the client code can create the zeros itself;
					// we don't need to hump them along inside the zarfile.
					uint32_t end = min(range->end, filelen);
					add_chunk(range->start, end - range->start, true, ce, &cc);
				}
			}
			else
			{
				add_chunk(0, len, true, ce, &cc);
				insert_elf_chunks(zf, ce, &cc);
				zf = NULL;	// insert_elf_chunks consumes zf
			}

			delete zf;
		}
		phdr->connect_chunks(cc.first_chunk_idx, cc.count);
	}
	zhdr->set_index_count(phdr_count);
}

void Catalog::place_chunks()
{
	while (true)
	{
		// try to fill up gaps at the end of a block with small files.
		while (true)
		{
			ChunkBySize key(placer->gap_size());
			ChunkBySize *cebs =
				chunk_tree.findFirstLessThanOrEqualTo(&key);
			if (cebs==NULL)
			{
				break;
			}
			ChunkEntry *next_small = cebs->get_chunk_entry();
			chunk_tree.remove(next_small);
#if DEBUG_SCHEDULER
			fprintf(stderr, "I had a gap 0x%08x; best fit was 0x%08x: %s\n",
				placer->gap_size(),
				next_small->get_size(),
				next_small->get_url());
#endif // DEBUG_SCHEDULER
// debug hooks
//			ChunkBySize *d_cebs = chunk_tree.findFirstLessThanOrEqualTo(&key);
//			int c = cebs->cmp(d_cebs);
//			(void) c;
			placer->place(next_small);
		}
		// if there are no more files small enough to fit in the remaining gap,
		// and there are still big files to come, add padding.
		ChunkBySize *cebs = chunk_tree.findMax();
		if (cebs==NULL)
		{
			// oh, we're done.
			break;
		}
		ChunkEntry *next_big = cebs->get_chunk_entry();
		chunk_tree.remove(next_big);
#if DEBUG_SCHEDULER
		fprintf(stderr, "Gap too small 0x%08x; found biggest 0x%08x: %s\n",
			placer->gap_size(),
			next_big->get_size(),
			next_big->get_url());
#endif // DEBUG_SCHEDULER
		if (placer->gap_size() > 0 && next_big->get_size()>ZFTP_BLOCK_SIZE)
		{
			uint32_t padding_size = placer->gap_size();
			dbg_total_padding += padding_size;
			placer->place(new Padding(padding_size));
		}
		placer->place(next_big);
	}
}
