#include <stdio.h>

#include "standard_malloc_factory.h"
#include "malloc_factory_operator_new.h"

class Bonk
{
public:
	Bonk() { fprintf(stderr, "Bonk ctor\n"); }
	~Bonk() { fprintf(stderr, "Bonk dtor\n"); }
};

int main()
{
	MallocFactory *mf = standard_malloc_factory_init();
	malloc_factory_operator_new_init(mf);
	
	Bonk *bonk = new Bonk();
	delete bonk;
}
