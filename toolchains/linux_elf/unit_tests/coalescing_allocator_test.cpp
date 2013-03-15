#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>

#include "CoalescingAllocator.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "malloc_factory_operator_new.h"
#include "SyncFactory_Pthreads.h"

class Customer : public UserObjIfc
{
public:
	Customer(char c) : c(c) {}
	char c;
};


class CATest
{
public:
	CATest();

	bool alloc(uint32_t size, char c);
	void free(uint32_t start, uint32_t end);
	void display();

	Range random_range();

private:
	void display_some(uint32_t start, uint32_t end, char c);
	void display_one_tree(ByStartTree *tree);

	Range full_range;
	CoalescingAllocator *allocator;
};

CATest::CATest()
	: full_range(1,79)
{
	MallocFactory *mf = standard_malloc_factory_init();
	malloc_factory_operator_new_init(mf);
	SyncFactory *sf = new SyncFactory_Pthreads();

	allocator = new CoalescingAllocator(sf, true);
	allocator->create_empty_range(full_range);
}

Range CATest::random_range()
{
	return Range(
		(random() % full_range.size())+full_range.start,
		(random() % full_range.size())+full_range.start);
}

bool CATest::alloc(uint32_t size, char c)
{
	Range out;
	bool rc = allocator->allocate_range(size, new Customer(c), &out);
	return rc;
}

void CATest::free(uint32_t start, uint32_t end)
{
	allocator->free_range(Range(start, end));
}

void CATest::display_some(uint32_t start, uint32_t end, char c)
{
	for (uint32_t i=start; i<end; i++)
	{
		putc(c, stdout);
	}
}

void CATest::display()
{
	display_one_tree(allocator->dbg_peek_allocated_tree());
	display_one_tree(allocator->dbg_peek_free_by_start_tree());
	fflush(stdout);
}

void CATest::display_one_tree(ByStartTree *tree)
{
	Range lastRange(0,1);

	const char *free_syms = "@#";
	int free_sym = 0;

	while (true)
	{
		RangeByStartKey lastKey(lastRange);
		RangeByStartElt *firstElt = tree->findFirstGreaterThan(lastKey);
		if (firstElt==NULL)
		{
			// no more allocated ranges.
			break;
		}

		Range thisRange = firstElt->getKey().get_range();
		lite_assert(lastRange.end <= thisRange.start);
		display_some(lastRange.end, thisRange.start, '.');
		Customer *customer = (Customer*) firstElt->get_user_obj();
		char c = (customer!=NULL) ? customer->c : free_syms[(free_sym++)&1];
		display_some(thisRange.start, thisRange.end, c);
		lastRange = thisRange;
	}
	display_some(lastRange.end, full_range.end, '.');
	putc('\n', stdout);
}

int main()
{
	CATest cat;
	cat.alloc(80, 'x');
	cat.display();
	cat.alloc(79, 'y');
	cat.display();
	cat.alloc(78, 'z');
	cat.display();
	cat.free(78,79);
	cat.display();
	cat.free(79,81);
	cat.display();
	cat.free(0,800);
	cat.display();
	cat.alloc(5, 'A');
	cat.display();
	cat.alloc(10, 'B');
	cat.display();
	cat.alloc(15, 'C');
	cat.display();
	cat.free(7,9);
	cat.display();
	cat.free(11,13);
	cat.display();
	cat.free(3,20);
	cat.display();
	cat.alloc(17, 'D');
	cat.display();

	int letter_idx = 4;
	for (int i=0; i<100000; i++)
	{
		letter_idx = (letter_idx+1)%26;
		Range a = cat.random_range();
		bool rc =cat.alloc(a.size(), 'A'+letter_idx);
		if (rc)
		{
			cat.display();
		}

		Range f = cat.random_range();
		if (f.size()==0)
		{
			continue;
		}
		cat.free(f.start, f.end);
		cat.display();
	}
}

