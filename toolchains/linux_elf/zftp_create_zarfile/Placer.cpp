#include <malloc.h>
#include "Placer.h"
#include "Padding.h"
#include "LiteLib.h"
#include "zftp_protocol.h"
#include "CatalogEntry.h"
#include "ChunkEntry.h"

int Placement::cmp(Placement *a, Placement *b)
{
	int r;
	r = a->range.start() - b->range.start();
	if (r!=0) { return r; }
	return 0;
}

int FreeToken::cmp(FreeToken *a, FreeToken *b)
{
	int r;
	r = a->range.size() - b->range.size();
	if (r!=0) { return r; }
	r = a->range.start() - b->range.start();	// disambiguate by start
	if (r!=0) { return r; }
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

Placer::Placer(MallocFactory *mf, uint32_t zftp_block_size)
{
	this->_mf = mf;
	this->zftp_block_size = zftp_block_size;
	total_size = 0;
	dbg_total_padding = 0;
	placement_tree = new PlacementTree(&sf);
	free_tree = new FreeTree(&sf);
	debugf = fopen("placements", "w");
}

Placer::~Placer()
{
	// Can't free anything until all emitting is done,
	// since ChunkTable (later) uses pointers to ChunkEntrys (earlier).
	delete_all();

	lite_assert(placement_tree->size()==0);
}

void Placer::add_space(uint32_t size)
{
	uint32_t start = 0;
	Placement* last = placement_tree->findMax();
	if (last!=NULL)
	{
		start = last->get_range().end(); 
	}
	DataRange free_space(start, start+size);
	placement_tree->insert(new Placement(free_space));
	free_tree->insert(new FreeToken(free_space));
}

bool Placer::place(Emittable *e, uint32_t location)
{
	//dbg_sanity();
	Placement* existing;
	Placement proposed(e, location);

	existing = placement_tree->findFirstLessThanOrEqualTo(&proposed);
	lite_assert(existing!=NULL);	// should at least be a placeholder here.
	if (!existing->is_free())
	{
		return false;
	}
	if (!(existing->get_range().end() >= proposed.get_range().end()))
	{
		return false;
	}

	placement_tree->remove(existing);
	Placement* realized = new Placement(e, location);
	placement_tree->insert(realized);
	e->set_location(realized->get_range().start());

	FreeToken key(existing->get_range());
	FreeToken* old_free = free_tree->remove(key);
	delete old_free;

	DataRange left_gap(
		existing->get_range().start(), realized->get_range().start());
	if (left_gap.size()>0)
	{
		free_tree->insert(new FreeToken(left_gap));
		placement_tree->insert(new Placement(left_gap));
	}
	DataRange right_gap(
		realized->get_range().end(), existing->get_range().end());
	if (right_gap.size()>0)
	{
		free_tree->insert(new FreeToken(right_gap));
		placement_tree->insert(new Placement(right_gap));
	}
	delete existing;
	return true;
}

void Placer::stretch(uint32_t size)
{
	// chop off any existing free tail
	Placement* last = placement_tree->findMax();
	if (last!=NULL && last->is_free())
	{
		FreeToken key(last->get_range());
		FreeToken* ft = free_tree->remove(key);
		delete ft;
		placement_tree->remove(last);
		delete last;
	}

	// create exactly as much space as is needed.
	last = placement_tree->findMax();
	DataRange newspace;
	if (last==NULL)
	{
		newspace = DataRange(0, size);
	}
	else
	{
		lite_assert(!last->is_free());
		newspace = DataRange(last->get_range().end(), last->get_range().end()+size);
	}
	
	// insert it as free space so that the place algorithm can continue
	// as in normal case.
	Placement* free_placement = new Placement(newspace);
	fprintf(debugf, "free_placement @%08x %08x %08x\n",
		(int) free_placement,
		free_placement->get_range().start(),
		free_placement->get_range().end());
	placement_tree->insert(free_placement);
	free_tree->insert(new FreeToken(newspace));
}

void Placer::place(Emittable *e)
{
	FreeToken key(DataRange(0, e->get_size()));
	FreeToken* best_free = free_tree->findFirstGreaterThanOrEqualTo(key);
	if (best_free==NULL)
	{
		stretch(e->get_size());
		best_free = free_tree->findFirstGreaterThanOrEqualTo(key);
		lite_assert(best_free != NULL);
	}
	lite_assert(best_free->get_range().size() >= e->get_size());

	free_tree->remove(best_free);
	Placement gap_key(best_free->get_range());
	Placement* old_gap = placement_tree->remove(gap_key);
	delete(old_gap);

	Placement* placement = new Placement(e, best_free->get_range().start());
	fprintf(debugf, "     placement @%08x %08x %08x\n",
		(int) placement,
		placement->get_range().start(),
		placement->get_range().end());
	placement_tree->insert(placement);
	e->set_location(placement->get_range().start());

	DataRange remainder(
		placement->get_range().end(), best_free->get_range().end());
	if (remainder.size()>0)
	{
		free_tree->insert(new FreeToken(remainder));
		Placement* remainder_pl = new Placement(remainder);
		placement_tree->insert(remainder_pl);
		fprintf(debugf, "remainder_pl   @%08x %08x %08x\n",
			(int) remainder_pl,
			remainder_pl->get_range().start(),
			remainder_pl->get_range().end());
	}
	//dbg_sanity();
}

#if 0
static uint32_t min(uint32_t a, uint32_t b)
{
	if (a<b) { return a; } else { return b; }
}

void Placer::emit_zeros(FILE* ofp, uint32_t size)
{
	int left = size;
	while (left > 0)
	{
		int group = min(left, 1<<16);
		uint8_t* zeros = (uint8_t*) malloc(group);
		memset(zeros, 0, group);
		int rc = fwrite(zeros, group, 1, ofp);
		lite_assert(rc==1);
		free(zeros);
		left -= group;
	}
}
#endif

class TypeAccumulator {
private:
	const char* _type;
	uint32_t _value;

public:
	TypeAccumulator(const char* type)
		: _type(type), _value(0) {}
	const char* get_type() { return _type; }
	uint32_t get_value() { return _value; }
	void add(uint32_t v) { _value += v; }

	static uint32_t hash(const void* v_a) {
		TypeAccumulator* a = (TypeAccumulator*) v_a;
		return hash_buf(a->_type, strlen(a->_type));
	}
	static int cmp(const void* v_a, const void* v_b) {
		TypeAccumulator* a = (TypeAccumulator*) v_a;
		TypeAccumulator* b = (TypeAccumulator*) v_b;
		return strcmp(a->_type, b->_type);
	}
};

void Placer::emit_one(Emittable* e)
{
	if (ofp!=NULL)
	{
		e->emit(ofp);
	}
	current_offset += e->get_size();

	TypeAccumulator key(e->get_type());
	TypeAccumulator* accum =
		(TypeAccumulator*) hash_table_lookup(&dbg_type_ht, &key);
	if (accum == NULL)
	{
		accum = new TypeAccumulator(e->get_type());
		hash_table_insert(&dbg_type_ht, accum);
	}
	accum->add(e->get_size());
}

void Placer::_dbg_print_one_type(void* v_this, void* v_a)
{
	TypeAccumulator* a = (TypeAccumulator*) v_a;
//	fprintf(stderr, "%s %d bytes\n", a->get_type(), a->get_value());
	(void)a;
}

void Placer::dbg_bytes_by_type(uint32_t total_bytes)
{
	hash_table_visit_every(&dbg_type_ht, _dbg_print_one_type, this);

	Padding dummy_padding(0);
	TypeAccumulator padding_key(dummy_padding.get_type());
	TypeAccumulator* padding_accum =
		(TypeAccumulator*) hash_table_lookup(&dbg_type_ht, &padding_key);
	uint32_t padding_bytes = 0;
	if (padding_accum!=NULL)
	{
		padding_bytes = padding_accum->get_value();
	}

	ChunkEntry dummy_chunk_entry(0, 0, ChunkEntry::PRECIOUS, NULL);
	TypeAccumulator chunk_entry_key(dummy_chunk_entry.get_type());
	TypeAccumulator* chunk_entry_accum =
		(TypeAccumulator*) hash_table_lookup(&dbg_type_ht, &chunk_entry_key);
	uint32_t chunk_entry_bytes = 0;
	if (chunk_entry_accum!=NULL)
	{
		chunk_entry_bytes = chunk_entry_accum->get_value();
	}

	fprintf(stderr, "total %d bytes\n", total_bytes);
	fprintf(stderr, "content %d bytes\n", chunk_entry_bytes);
	fprintf(stderr, "padding %d bytes\n", padding_bytes);
	fprintf(stderr, "metadata %d bytes\n", total_bytes-padding_bytes-chunk_entry_bytes);
}

void Placer::emit(const char *out_path)
{
	hash_table_init(&dbg_type_ht, _mf,
		TypeAccumulator::hash, TypeAccumulator::cmp);

	ofp = NULL;
	if (out_path!=NULL)
	{
		ofp = fopen(out_path, "wb");
		lite_assert(ofp!=NULL);
	}
	current_offset = 0;

	Placement* placement = placement_tree->findMin();
	while (placement!=NULL)
	{
		lite_assert(placement->get_range().start() == current_offset);
		Emittable* e;
		if (placement->is_free())
		{
			e = new Padding(placement->get_range().size());	// TODO leak
		}
		else
		{
			e = placement->get_emittable();
		}
		emit_one(e);
		placement = placement_tree->findFirstGreaterThan(placement);
	}
	lite_assert(ofp==NULL || ftell(ofp)==(int)current_offset);

	total_size = current_offset;

	if (ofp!=NULL)
	{
		fclose(ofp);
	}

	dbg_bytes_by_type(total_size);
}

void Placer::delete_all()
{
	Placement* placement = placement_tree->findMin();
	while (placement!=NULL)
	{
		placement_tree->remove(placement);
		delete placement->get_emittable();
		delete placement;
		placement = placement_tree->findMin();
	}
}

void Placer::pad(uint32_t padding_size)
{
	dbg_total_padding += padding_size;
	place(new Padding(padding_size));
}

void Placer::dbg_report()
{
	fprintf(stderr, "Total padding used: %.1fkB (%.3f%%, %d bytes)\n",
		dbg_total_padding*1.0/(1<<10),
		dbg_total_padding*100.0/get_total_size(),
		dbg_total_padding);
}

uint32_t Placer::get_offset()
{
	Placement* last = placement_tree->findMax();
	if (last==NULL)
	{
		return 0;
	}
	if (last->is_free())
	{
		last = placement_tree->findFirstLessThan(last);
		lite_assert(!last->is_free());
	}
	return last->get_range().end();
}

uint32_t Placer::gap_size()
{
	uint32_t used = get_offset() & (zftp_block_size-1);
	if (used==0)
	{
		return 0;	// no, you don't get *another* block!
	}
	return zftp_block_size - used;
}

#if 0
void Placer::dbg_sanity()
{
	uint32_t current_offset = 0;
	Placement* placement = placement_tree->findMin();
	Placement* dbg_prev = NULL;
	while (placement!=NULL)
	{
		lite_assert(placement->get_range().start() == current_offset);
		current_offset = placement->get_range().end();
		dbg_prev = placement;
		placement = placement_tree->findFirstGreaterThan(placement);
	}
	fprintf(stderr, "Tree contiguous 0..%08x\n", current_offset);
}
#endif
