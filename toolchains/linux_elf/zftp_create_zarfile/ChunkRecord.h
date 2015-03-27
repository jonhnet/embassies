#pragma once

#include <stdint.h>
#include "avl2.h"
#include "DataRange.h"

class ChunkRecord {
private:
	char* url;
	bool own_url;
	DataRange data_range;
	uint32_t location;

public:
	ChunkRecord(const char* url, bool own_url, DataRange data_range, uint32_t location);
	ChunkRecord(const ChunkRecord& other);
	ChunkRecord& operator=(ChunkRecord other);

	~ChunkRecord();
	ChunkRecord getKey() { return ChunkRecord(*this); }
	const char* get_url() { return url; }
	DataRange get_data_range() { return data_range; }
	uint32_t get_location() { return location; }

	int cmp(ChunkRecord *other);
	bool content_equals(ChunkRecord *other);

	bool operator<(ChunkRecord b)
		{ return cmp(&b) <0; }
	bool operator>(ChunkRecord b)
		{ return cmp(&b) >0; }
	bool operator==(ChunkRecord b)
		{ return cmp(&b) ==0; }
	bool operator<=(ChunkRecord b)
		{ return cmp(&b) <=0; }
	bool operator>=(ChunkRecord b)
		{ return cmp(&b) >=0; }
};

typedef AVLTree<ChunkRecord,ChunkRecord> ChunkRecordTree;

