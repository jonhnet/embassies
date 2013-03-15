#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>

#include "avl2.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "malloc_factory_operator_new.h"
#include "SyncFactory_Pthreads.h"

class IntElement
{
public:
	IntElement(const IntElement &proto) { this->value = proto.value; }
	IntElement(int value) { this->value = value; }
	IntElement getKey() { return *this; }
	bool operator<(IntElement b)
		{ return value < b.value; }
	bool operator>(IntElement b)
		{ return value > b.value; }
	bool operator==(IntElement b)
		{ return value == b.value; }
	bool operator<=(IntElement b)
		{ return value <= b.value; }
	bool operator>=(IntElement b)
		{ return value >= b.value; }
private:
	int value;
};

typedef AVLTree<IntElement,IntElement> IntTree;

int main()
{
	MallocFactory *mf = standard_malloc_factory_init();
	malloc_factory_operator_new_init(mf);
	SyncFactory *sf = new SyncFactory_Pthreads();

	IntTree tree(sf);

	int dups = 0;
	const unsigned num_insert_attempts = 1000000;
	for (unsigned i=0; i<num_insert_attempts; i++)
	{
		int v = random();
		IntElement *box = new IntElement(v);
		if (tree.lookup(box)!=NULL)
		{
			dups += 1;
			delete box;
		}
		else
		{
			bool rc = tree.insert(box);
			lite_assert(rc);
		}
	}

	fprintf(stderr, "skipped insert of %d dup values.\n", dups);

	tree.check();
	fprintf(stderr, "tree size %d\n", tree.size());

	lite_assert(num_insert_attempts == tree.size()+dups);

	IntElement *prev = NULL;
	while (!tree.empty())
	{
		IntElement *next = tree.findMin();
		tree.remove(next);
		if (prev!=NULL)
		{
			lite_assert(*prev < *next);
			delete prev;
		}
		prev = next;
	}
	delete prev;
}

