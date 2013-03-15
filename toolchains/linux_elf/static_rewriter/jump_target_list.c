#include <assert.h>

#include "jump_target_list.h"

void jump_target_list_init(JumpTargetList *jtl)
{
	jtl->count = 0;
	jtl->capacity = sizeof(jtl->targets)/sizeof(jtl->targets[0]);
}

void jump_target_list_add(JumpTargetList *jtl, int pos)
{
	assert(jtl->count < jtl->capacity);
	jtl->targets[jtl->count] = pos;
	jtl->count+=1;
}
