#include "SmallIntegerAllocator.h"

SmallIntegerBox::SmallIntegerBox(uint32_t value)
{
	this->value = value;
}

SmallIntegerAllocator::SmallIntegerAllocator(MallocFactory *mf, uint32_t first_free_value)
{
	this->next_free_value = first_free_value;
	linked_list_init(&free_values, mf);
}

uint32_t SmallIntegerAllocator::allocate()
{
	uint32_t result;
	SmallIntegerBox *box = (SmallIntegerBox *) linked_list_remove_head(&free_values);
	if (box!=NULL)
	{
		result = box->value;
		delete box;
	}
	else
	{
		result = this->next_free_value;
		this->next_free_value+=1;
	}
	return result;
}

void SmallIntegerAllocator::free(uint32_t value)
{
	SmallIntegerBox *box = new SmallIntegerBox(value);
	linked_list_insert_head(&free_values, box);
}

