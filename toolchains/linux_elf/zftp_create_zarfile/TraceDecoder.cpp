#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "TraceDecoder.h"
#include "TraceDecoder.h"
#include "standard_malloc_factory.h"
#include "LiteLib.h"
#include "cheesy_snprintf.h"

IdMapping::IdMapping(uint32_t id, const char* path, MallocFactory *mf)
	: id(id),
	  path(mf_strdup(mf, path)),
	  mf(mf)
{}

IdMapping::IdMapping(uint32_t id)
	: id(id),
	  path(NULL),
	  mf(NULL)
{}

IdMapping::~IdMapping()
{
	if (path!=NULL)
	{
		mf_free(mf, path);
	}
}

const char* IdMapping::get_path()
{
	return path;
}

uint32_t IdMapping::hash(const void* datum)
{
	IdMapping* obj = (IdMapping*) datum;
	return obj->id;
}

int IdMapping::cmp(const void *v_a, const void *v_b)
{
	IdMapping* a = (IdMapping*) v_a;
	IdMapping* b = (IdMapping*) v_b;
	return a->id - b->id;
}

//////////////////////////////////////////////////////////////////////////////

TracePathRecord::TracePathRecord(const char* path, MallocFactory *mf, SyncFactory* sf, KeyFlag key_flag)
	: key_flag(key_flag),
	  path(mf_strdup(mf, path)),
	  mf(mf),
	  precious_chunks(NULL)
{
	if (key_flag==RECORD)
	{
		linked_list_init(&fast_chunks, mf);
		linked_list_init(&deferred_reads, mf);
		precious_chunks = new RangeUnion(sf);
	}
}

TracePathRecord::~TracePathRecord()
{
	if (key_flag==RECORD)
	{
		mf_free(mf, path);
		delete(precious_chunks);
	}
}

uint32_t TracePathRecord::hash(const void* datum)
{
	TracePathRecord* obj = (TracePathRecord*) datum;
	return hash_buf(obj->path, strlen(obj->path));
}

int TracePathRecord::cmp(const void *v_a, const void *v_b)
{
	TracePathRecord* a = (TracePathRecord*) v_a;
	TracePathRecord* b = (TracePathRecord*) v_b;
	return strcmp(a->path, b->path);
}

void TracePathRecord::add_mmap(uint32_t size, uint32_t offset, bool fast, uint32_t sequence_number)
{
	if (!fast)
	{
		add_read(size, offset, sequence_number);
	}
	else
	{
		lite_assert(offset==0);
			// pretty sure fast is never used except at beginning of a file.
			// (If this assumption is wrong, a likely-reasonable fix is just
			// to downgrade offset!=0 to fast=false.)
		FastChunk* chunk = new FastChunk(size, sequence_number);
		linked_list_insert_tail(&fast_chunks, chunk);
	}
}

void TracePathRecord::add_read(uint32_t size, uint32_t offset, uint32_t sequence_number)
{
	linked_list_insert_tail(&deferred_reads, new ReadRecord(size, offset, sequence_number));
}

bool TracePathRecord::fast_absorbs_read(ReadRecord* rr)
{
	LinkedListIterator lli;
	for (ll_start(&fast_chunks, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		FastChunk* fast_chunk = (FastChunk*) ll_read(&lli);
		if (fast_chunk->sequence_number < rr->sequence_number)
		{
			// sorry, that fast chunk will be gone by the time this
			// read is needed.
			continue;
		}
		if (fast_chunk->range.intersect(rr->range).size()
			== rr->range.size())
		{
			// complete overlap. Yay!
			return true;
		}
	}
	// Note that this code cannot detect the case where two non-consumed
	// fast chunks together might resolve a prior read. That's not possible
	// at the time of this writing in any case, since all fast chunks start
	// at offset 0 anyway (the common case; the .text segment).
	return false;
}

void TracePathRecord::integrate_reads()
{
	while (deferred_reads.count>0)
	{
		ReadRecord* rr = (ReadRecord*) linked_list_remove_head(&deferred_reads);
		if (!fast_absorbs_read(rr))
		{
			precious_chunks->accumulate(rr->range);
		}
		delete rr;
	}
}

//////////////////////////////////////////////////////////////////////////////

TraceDecoder::TraceDecoder(const char* trace_filename, MallocFactory* mf, SyncFactory* sf)
	: mf(mf),
	  sf(sf),
	  broken(false)
{
	hash_table_init(&id_mappings, mf, IdMapping::hash, IdMapping::cmp);
	hash_table_init(&path_records, mf, TracePathRecord::hash, TracePathRecord::cmp);

	parse(trace_filename);
}

bool field_equals(const char* a, const char* b)
{
	int lena = strlen(a);
	int lenb = strlen(b);
	return lena >= lenb
		&& memcmp(a, b, lenb)==0;
}

void TraceDecoder::parse(const char* trace_filename)
{
	FILE *fp = fopen(trace_filename, "r");
	if (fp==NULL)
	{
		fprintf(stderr, "WARNING: No mmap cue file '%s' present.\n",
			trace_filename);
		return;
	}

	char buf[800];
	const char* fields[10];
	uint32_t sequence_number = 0;
	while (true) {
		char *line = fgets(buf, sizeof(buf), fp);
		if (line==NULL)
		{
			break;
		}

		lite_assert(strlen(line)+1<sizeof(buf));	// overflow!
		lite_assert(line[strlen(line)-1]=='\n');
		line[strlen(line)-1]='\0';
		int max_fields = sizeof(fields)/sizeof(fields[0]);
		int num_fields = lite_split(buf, ' ', fields, max_fields);
		lite_assert(num_fields < max_fields);	// overflow! Too many fields.

		if (field_equals(fields[0], "OPEN"))
		{
			parse_open(fields, num_fields);
		}
		else if (field_equals(fields[0], "READ"))
		{
			parse_read(fields, num_fields, sequence_number);
		}
		else if (field_equals(fields[0], "MMAP"))
		{
			parse_mmap(fields, num_fields, sequence_number);
		}
		else
		{
			lite_assert(false);
			broken = true;
		}
		sequence_number += 1;
	}
	lite_assert(!broken);

	// Now coalesce overlapping reads into a nonoverlapping sequence of
	// ranges. This will remove duplicates, and (most importantly) remove
	// reads that can be satisfied by a fast section that hasn't yet
	// been consumed by the time the read occurs.
	hash_table_visit_every(&path_records, s_integrate_reads, this);
}

void TraceDecoder::parse_open(const char** fields, int num_fields)
{
	lite_assert(num_fields>=3);	// >3 means the filename has a space in it, but we can cope with that.
	uint32_t id = str2hex(fields[1]);
	const char* path = fields[2];	// NB I happen to know the last field is zero-terminatod.

	lite_assert(!lite_ends_with(".zarfile", path));
		// That's too recurse-y.

	TracePathRecord key(path, mf, NULL, TracePathRecord::KEY);
	TracePathRecord* rec =
		(TracePathRecord*) hash_table_lookup(&path_records, &key);
	if (rec==NULL)
	{
		rec = new TracePathRecord(path, mf, sf, TracePathRecord::RECORD);
		hash_table_insert(&path_records, rec);
	}
	IdMapping* idm = new IdMapping(id, path, mf);
	hash_table_insert(&id_mappings, idm);
}

TracePathRecord* TraceDecoder::id_to_record(uint32_t id)
{
	IdMapping i_key(id);
	IdMapping* idm = (IdMapping*) hash_table_lookup(&id_mappings, &i_key);
	lite_assert(idm!=NULL);	// huh? id referenced before OPEN
	TracePathRecord p_key(idm->get_path(), mf, NULL, TracePathRecord::KEY);
	TracePathRecord* rec =
		(TracePathRecord*) hash_table_lookup(&path_records, &p_key);
	lite_assert(rec!=NULL);	// OPEN should have created id record
	return rec;
}

void TraceDecoder::parse_read(const char** fields, int num_fields, uint32_t sequence_number)
{
	lite_assert(num_fields==4);
	uint32_t id = str2hex(fields[1]);
	uint32_t size = str2hex(fields[2]);
	uint32_t offset = str2hex(fields[3]);

	TracePathRecord* rec = id_to_record(id);
	rec->add_read(size, offset, sequence_number);

//	_dbg_dump_foreach(this, rec);
}

void TraceDecoder::parse_mmap(const char** fields, int num_fields, uint32_t sequence_number)
{
	lite_assert(num_fields==5);
	uint32_t id = str2hex(fields[1]);
	uint32_t size = str2hex(fields[2]);
	uint32_t offset = str2hex(fields[3]);
	uint32_t fast = str2hex(fields[4]);

	id_to_record(id)->add_mmap(size, offset, fast, sequence_number);
}

void TraceDecoder::_dbg_dump_foreach(void* v_this, void* v_rec)
{
	TracePathRecord* rec = (TracePathRecord*) v_rec;
	fprintf(stderr, "%s:\n", rec->path);

	LinkedListIterator lli;
	for (ll_start(&rec->fast_chunks, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		FastChunk* fast_chunk = (FastChunk*) ll_read(&lli);
		fprintf(stderr, "  fast len %08x\n", fast_chunk->range.size());
	}

	Range cursor;
	bool found;
	for (found = rec->precious_chunks->first_range(&cursor);
		found;
		found = rec->precious_chunks->next_range(cursor, &cursor))
	{
		fprintf(stderr, "  read @%08x len %08x\n", cursor.start, cursor.size());
	}
}

void TraceDecoder::dbg_dump()
{
	fprintf(stderr, "TraceDecoder has info on %d paths:\n", path_records.count);
	hash_table_visit_every(&path_records, _dbg_dump_foreach, this);
}

void TraceDecoder::visit_trace_path_records(ForeachFunc* ff, void* v_that)
{
	hash_table_visit_every(&path_records, ff, v_that);
}

void TraceDecoder::s_integrate_reads(void* v_this, void* v_rec)
{
	TracePathRecord* rec = (TracePathRecord*) v_rec;
	rec->integrate_reads();
}

TracePathRecord* TraceDecoder::lookup(const char* path)
{
	TracePathRecord p_key(path, mf, NULL, TracePathRecord::KEY);
	TracePathRecord* rec =
		(TracePathRecord*) hash_table_lookup(&path_records, &p_key);
	return rec;
}
