#ifndef _JUMP_TARGET_LIST_H
#define _JUMP_TARGET_LIST_H

typedef struct {
	int count;
	int capacity;
	int targets[1];
} JumpTargetList;

void jump_target_list_init(JumpTargetList *jtl);
void jump_target_list_add(JumpTargetList *jtl, int pos);

#endif // _JUMP_TARGET_LIST_H
