#include "Table.h"
#include "LiteLib.h"

class Tuple
{
public:
	Tuple(Hashable *l, Hashable *r);
	static uint32_t hash_left(const void *v_a);
	static int cmp_lefts(const void *v_a, const void *v_b);
	static uint32_t hash_right(const void *v_a);
	static int cmp_rights(const void *v_a, const void *v_b);

	Hashable *left;
	Hashable *right;
};

Tuple::Tuple(Hashable *l, Hashable *r)
	: left(l), right(r) {}

uint32_t Tuple::hash_left(const void *v_a)
{
	return ((Tuple*) v_a)->left->hash();
}

uint32_t Tuple::hash_right(const void *v_a)
{
	return ((Tuple*) v_a)->right->hash();
}

int Tuple::cmp_lefts(const void *v_a, const void *v_b)
{
	return ((Tuple*) v_a)->left->cmp(((Tuple*) v_b)->left);
}

int Tuple::cmp_rights(const void *v_a, const void *v_b)
{
	return ((Tuple*) v_a)->right->cmp(((Tuple*) v_b)->right);
}

//////////////////////////////////////////////////////////////////////////////

Table::Table(MallocFactory *mf)
{
	hash_table_init(&l_to_r, mf, Tuple::hash_left, Tuple::cmp_lefts);
	hash_table_init(&r_to_l, mf, Tuple::hash_right, Tuple::cmp_rights);
}

void Table::insert(Hashable *hl, Hashable *hr)
{
	Tuple *tuple = new Tuple(hl, hr);
	hash_table_insert(&l_to_r, tuple);
	hash_table_insert(&r_to_l, tuple);
}

Hashable *Table::lookup_left(Hashable *hl)
{
	Tuple key(hl, NULL);
	Tuple *tuple = (Tuple *) hash_table_lookup(&l_to_r, &key);
	if (tuple==NULL)
	{
		return NULL;
	}
	return tuple->right;
}

Hashable *Table::lookup_right(Hashable *hr)
{
	Tuple key(NULL, hr);
	Tuple *tuple = (Tuple *) hash_table_lookup(&r_to_l, &key);
	if (tuple==NULL)
	{
		return NULL;
	}
	return tuple->left;
}

void Table::remove_left(Hashable *hl)
{
	Tuple key(hl, NULL);
	Tuple *tuple = (Tuple *) hash_table_lookup(&l_to_r, &key);
	lite_assert(tuple!=NULL);
	hash_table_remove(&l_to_r, tuple);
	hash_table_remove(&r_to_l, tuple);
}

void Table::remove_right(Hashable *hr)
{
	Tuple key(NULL, hr);
	Tuple *tuple = (Tuple *) hash_table_lookup(&r_to_l, &key);
	lite_assert(tuple!=NULL);
	hash_table_remove(&l_to_r, tuple);
	hash_table_remove(&r_to_l, tuple);
}
