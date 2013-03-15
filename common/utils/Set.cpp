#include "Set.h"
#include "LiteLib.h"

Set::Set(MallocFactory *mf)
{
	this->mf = mf;
	hash_table_init(&ht, mf, Hashable::_hash, Hashable::_cmp);
}

void Set::insert(Hashable *h)
{
	if (contains(h))
	{
		lite_assert(false);
	}
	hash_table_insert(&ht, h);
}

bool Set::contains(Hashable *h)
{
	return lookup(h)!=NULL;
}

Hashable* Set::lookup(Hashable *h)
{
	Hashable* h_found = (Hashable*) hash_table_lookup(&ht, h);
	return h_found;
}

uint32_t Set::count()
{
	return ht.count;
}

class Intersect {
private:
	Set *b;
	Set *out;
	bool invert;
public:
	Intersect(Set *b, Set *out, bool invert) : b(b), out(out), invert(invert) {}
		// !invert => intersection,
		// invert => subtraction
	static void visit(void *user_obj, void *v_a);
};

void Intersect::visit(void *user_obj, void *v_a)
{
	Intersect *i = (Intersect *) user_obj;
	Hashable *h = (Hashable *) v_a;
	bool result = i->b->contains(h);
	if (i->invert) { result = !result; }
	if (result)
	{
		i->out->insert(h);
	}
}

Set *Set::intersection(Set *other)
{
	Set *out = new Set(mf);
	Intersect i(other, out, false);
	hash_table_visit_every(&ht, i.visit, &i);
	return out;
}

Set *Set::difference(Set *other)
{
	Set *out = new Set(mf);
	Intersect i(other, out, true);
	hash_table_visit_every(&ht, i.visit, &i);
	return out;
}

void Set::visit(ForeachFunc* ff, void* user_obj)
{
	hash_table_visit_every(&ht, ff, user_obj);
}
