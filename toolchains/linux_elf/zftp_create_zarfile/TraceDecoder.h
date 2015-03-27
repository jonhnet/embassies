#pragma once
#include "hash_table.h"
#include "linked_list.h"
#include "malloc_factory.h"
#include "RangeUnion.h"

class IdMapping {
private:
	uint32_t id;
	char* path;
	MallocFactory *mf;

public:
	IdMapping(uint32_t id, const char* path, MallocFactory *mf);
	IdMapping(uint32_t id);	// key ctor
	~IdMapping();
	const char* get_path();

	static uint32_t hash(const void* datum);
	static int cmp(const void *v_a, const void *v_b);
};

class FastChunk {
public:
	Range range;
	uint32_t sequence_number;
	// Sequence number used to decide which FastChunks can absorb wihch
	// ReadRecords.

	// can exceed file length if we are to include a range of zeros after.
	FastChunk(uint32_t size, uint32_t sequence_number)
		: range(0, size),
		  sequence_number(sequence_number) {}
};

class ReadRecord
{
public:
	Range range;
	uint32_t sequence_number;

	ReadRecord(uint32_t size, uint32_t offset, uint32_t sequence_number)
		: range(offset, offset+size),
		  sequence_number(sequence_number) {}
};

class TracePathRecord {
private:
	bool fast_absorbs_read(ReadRecord* rr);

public:
	enum KeyFlag { KEY, RECORD };

	KeyFlag key_flag;
	char* path;
	MallocFactory *mf;
	LinkedList fast_chunks;		// LinkedList<FastChunk>
	LinkedList deferred_reads;	// LinkedList<ReadRecords>
	RangeUnion* precious_chunks;

	TracePathRecord(const char* path, MallocFactory *mf);
	TracePathRecord(const char* path, MallocFactory *mf, SyncFactory* sf, KeyFlag key_flag);
	~TracePathRecord();

	static uint32_t hash(const void* datum);
	static int cmp(const void *v_a, const void *v_b);

	void add_mmap(uint32_t size, uint32_t offset, bool fast, uint32_t sequence_number);
	void add_read(uint32_t size, uint32_t offset, uint32_t sequence_number);
	void integrate_reads();
};

class TraceDecoder {
private:
	MallocFactory* mf;
	SyncFactory* sf;
	HashTable id_mappings;
	HashTable path_records;
	bool broken;

	void parse(const char* debug_mmap_filename);
	void parse_open(const char** fields, int num_fields);
	TracePathRecord* id_to_record(uint32_t id);
	void parse_read(const char** fields, int num_fields, uint32_t sequence_number);
	void parse_mmap(const char** fields, int num_fields, uint32_t sequence_number);
	static void _dbg_dump_foreach(void* v_this, void* v_behavior);
	static void s_integrate_reads(void* v_this, void* v_rec);

public:
	TraceDecoder(const char* trace_filename, MallocFactory* mf, SyncFactory* sf);
	void visit_trace_path_records(ForeachFunc* ff, void* v_that);
	TracePathRecord* lookup(const char* path);
	void dbg_dump();
};
