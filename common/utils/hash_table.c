#include "hash_table.h"
#include "LiteLib.h"
#include "math_util.h"

#define INITIAL_NUM_BUCKETS	3
#define LOAD_FACTOR_LIMIT	3

Subtable *subtable_init(HashTable *ht, int num_buckets)
{
	Subtable *st = (Subtable *)mf_malloc(ht->mf, sizeof(Subtable));
	st->buckets = (HashLink*)mf_malloc(ht->mf, sizeof(HashLink)*num_buckets);
	lite_memset(st->buckets, 0, sizeof(HashLink)*num_buckets);
	st->num_buckets = num_buckets;
	st->count = 0;
	return st;
}

void subtable_free(HashTable *ht, Subtable *st)
{
	int i;
	for (i=0; i<st->num_buckets; i++)
	{
		lite_assert(st->buckets[i].next == NULL);
	}
	mf_free(ht->mf, st);
}

int _subtable_find_bucket(HashTable *ht, Subtable *st, void *datum)
{
	uint32_t raw_hash = ht->hash_func(datum);
	return raw_hash % st->num_buckets;
}

HashLink *_subtable_find_preceding_link(HashTable *ht, Subtable *st, void *datum)
{
	int bucket_idx = _subtable_find_bucket(ht, st, datum);
	HashLink *hl = &st->buckets[bucket_idx];
	for (; hl->next!=NULL; hl=hl->next)
	{
		if (ht->cmp_func(datum, hl->next->datum)==0)
		{
			return hl;
		}
	}
	return NULL;
}

#if UNIT_TEST
#include <stdio.h>

void _hash_table_dump_unit_test_subtable(Subtable *st)
{
	int b;
	for (b=0; b<st->num_buckets; b++)
	{
		printf("    %2d: ", b);
		HashLink *hl = st->buckets[b].next;
		for (; hl!=NULL; hl=hl->next)
		{
			printf("%d", ((int*)hl->datum)[0]);
			if (hl->next!=NULL)
			{
				printf(", ");
			}
		}
		printf("\n");
	}
}

void _hash_table_debug_inner(HashTable *ht, const char *opn)
{
	printf("after %s:\n", opn);
	printf("  ht->old_table->count = %d\n", ht->old_table->count);
	_hash_table_dump_unit_test_subtable(ht->old_table);
	printf("  ht->new_table->count = %d\n", ht->new_table->count);
	_hash_table_dump_unit_test_subtable(ht->new_table);
	printf("  ht->count = %d\n", ht->count);
	fflush(stdout);
}

static inline void _hash_table_debug(HashTable *ht, const char *opn) {}
#else
static inline void _hash_table_debug(HashTable *ht, const char *opn) {}
#endif // UNIT_TEST

void hash_table_init(HashTable *ht, MallocFactory *mf, HashFunc *hash_func, CmpFunc *cmp_func)
{
	ht->mf = mf;
	ht->hash_func = hash_func;
	ht->cmp_func = cmp_func;
	ht->count = 0;
	ht->visiting = 0;

	ht->old_table = subtable_init(ht, 1);
	ht->next_old_bucket = 1;	// already empty
	ht->new_table = subtable_init(ht, 2);

	_hash_table_debug(ht, "hash_table_init");
}

void hash_table_free(HashTable *ht)
{
	lite_assert(ht->count==0);
	subtable_free(ht, ht->old_table);
	subtable_free(ht, ht->new_table);
	lite_memset(ht, 0, sizeof(*ht));
}

static void _hash_table_discard_worker(void *v_this, void *datum)
{
}

void hash_table_free_discard(HashTable *ht)
{
	hash_table_remove_every(ht, _hash_table_discard_worker, NULL);
	hash_table_free(ht);
}

void _hash_table_grow(HashTable *ht)
{
	subtable_free(ht, ht->old_table);	// expect it to be empty by now
	ht->old_table = ht->new_table;
	ht->new_table = subtable_init(ht, ht->old_table->num_buckets*2);
	ht->next_old_bucket = 0;
}

HashLink *_hash_table_find_preceding_link(HashTable *ht, void *datum, Subtable **in_table)
{
	HashLink *hl;
	*in_table = ht->new_table;
	hl = _subtable_find_preceding_link(ht, *in_table, datum);
	if (hl==NULL)
	{
		*in_table = ht->old_table;
		hl = _subtable_find_preceding_link(ht, *in_table, datum);
	}
	return hl;
}

void _hash_table_shift_one(HashTable *ht)
{
	HashLink *pred = NULL;
	while (ht->next_old_bucket < ht->old_table->num_buckets)
	{
		pred = &ht->old_table->buckets[ht->next_old_bucket];
		if (pred->next==NULL)
		{
#if UNIT_TEST && 0
			printf("_hash_table_shift_one: bucket %d is empty.\n", ht->next_old_bucket);
#endif // UNIT_TEST
			ht->next_old_bucket++;
			pred = NULL;
			continue;
		}
		break;
	}
	if (pred==NULL)
	{
		// old table is empty
#if UNIT_TEST && 0
		printf("_hash_table_shift_one: old table is empty\n");
#endif // UNIT_TEST
		return;
	}

	HashLink *victim = pred->next;
	pred->next = victim->next;
	ht->old_table->count -= 1;

	int bucket_idx = _subtable_find_bucket(ht, ht->new_table, victim->datum);
	HashLink *new_head = &ht->new_table->buckets[bucket_idx];
	victim->next = new_head->next;
	new_head->next = victim;
	ht->new_table->count += 1;
}

void hash_table_insert_inner(HashTable *ht, void *datum, bool assert_absent)
{
	lite_assert(ht->visiting==0);
	if (hash_table_lookup(ht, datum)!=NULL)
	{
		lite_assert(!assert_absent);
		return;
	}

	int bucket_idx = _subtable_find_bucket(ht, ht->new_table, datum);

	HashLink *bucket_head = &ht->new_table->buckets[bucket_idx];
	HashLink *new_link = (HashLink *) mf_malloc(ht->mf, sizeof(HashLink));
	new_link->magic0 = HLMAGIC0;
	new_link->next = bucket_head->next;
	new_link->datum = datum;
	new_link->magic1 = HLMAGIC1;
	bucket_head->next = new_link;

	ht->new_table->count += 1;
	ht->count += 1;

	int load_factor = ht->count / ht->new_table->num_buckets;
	if (load_factor > LOAD_FACTOR_LIMIT)
	{
		// hey, it's getting crowded in here. Time to grow the hash table.
		_hash_table_grow(ht);
	}

	_hash_table_shift_one(ht);

	_hash_table_debug(ht, "hash_table_insert");
}

void hash_table_insert(HashTable *ht, void *datum)
{
	hash_table_insert_inner(ht, datum, true);
}

void hash_table_union(HashTable *ht, void *datum)
{
	hash_table_insert_inner(ht, datum, false);
}

void *hash_table_lookup(HashTable *ht, void *key)
{
	Subtable *in_table;
	HashLink *pred = _hash_table_find_preceding_link(ht, key, &in_table);
	if (pred==NULL)
	{
		return NULL;
	}
	HashLink *target = pred->next;

	_hash_table_shift_one(ht);
	_hash_table_debug(ht, "hash_table_lookup");

	return target->datum;
}

void hash_table_remove(HashTable *ht, void *key)
{
	lite_assert(ht->visiting==0);
	Subtable *in_table;
	HashLink *pred = _hash_table_find_preceding_link(ht, key, &in_table);
	lite_assert(pred!=NULL /* expected key to be present */);
	HashLink *link = pred->next;
	pred->next = link->next;
	in_table->count -= 1;
	lite_assert(link->magic0==HLMAGIC0);
	lite_assert(link->magic1==HLMAGIC1);
	mf_free(ht->mf, link);
	ht->count -= 1;
	_hash_table_debug(ht, "hash_table_remove");
}

void _hash_table_visit_every_inner(HashTable *ht, Subtable *st, ForeachFunc *ff, void *user_obj)
{
	int bucket;
	for (bucket = 0; bucket < st->num_buckets; bucket++)
	{
		HashLink *hl = st->buckets[bucket].next;
		while (hl!=NULL)
		{
			ff(user_obj, hl->datum);
			hl = hl->next;
		}
	}
}

void hash_table_visit_every(HashTable *ht, ForeachFunc *ff, void *user_obj)
{
	ht->visiting += 1;
	_hash_table_visit_every_inner(ht, ht->old_table, ff, user_obj);
	_hash_table_visit_every_inner(ht, ht->new_table, ff, user_obj);
	ht->visiting -= 1;
}

void _hash_table_remove_every_inner(HashTable *ht, Subtable *st, ForeachFunc *ff, void *user_obj)
{
	int bucket;
	for (bucket = 0; bucket < st->num_buckets; bucket++)
	{
		HashLink *hl = st->buckets[bucket].next;
		st->buckets[bucket].next = NULL;
		while (hl!=NULL)
		{
			ff(user_obj, hl->datum);
			HashLink *next = hl->next;
			lite_assert(hl->magic0==HLMAGIC0);
			lite_assert(hl->magic1==HLMAGIC1);
			mf_free(ht->mf, hl);
			hl = next;
			st->count -= 1;
			ht->count -= 1;
		}
	}
}

void hash_table_remove_every(HashTable *ht, ForeachFunc *ff, void *user_obj)
{
	_hash_table_remove_every_inner(ht, ht->old_table, ff, user_obj);
	_hash_table_remove_every_inner(ht, ht->new_table, ff, user_obj);
	lite_assert(ht->count == 0);
	_hash_table_debug(ht, "hash_table_remove_every");
}

static void _hash_table_subtract_worker(void *v_this, void *datum)
{
	HashTable *target = (HashTable *) v_this;
	if (hash_table_lookup(target, datum)!=NULL)
	{
		hash_table_remove(target, datum);
	}
}

void hash_table_subtract(HashTable *ht0, HashTable *ht1)
{
	// assert that the HashTables agree on member type
	lite_assert(ht0->hash_func == ht1->hash_func);
	lite_assert(ht0->cmp_func == ht1->cmp_func);
	hash_table_visit_every(ht1, _hash_table_subtract_worker, ht0);
}

uint32_t hash_buf(const void *buf, int len)
{
	uint32_t v = 0;
	uint8_t *p = (uint8_t*) buf;
	int i;
	for (i=0; i<len; i++)
	{
		v = v*131 + p[i];
	}
	return v;
}

uint32_t cmp_bufs(const void *buf0, int len0, const void *buf1, int len1)
{
	int common_len = min(len0, len1);
	int rc = lite_memcmp(buf0, buf1, common_len);
	if (rc!=0)
	{
		return rc;
	}
	return len0-len1;
}
