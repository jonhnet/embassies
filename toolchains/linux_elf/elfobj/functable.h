#ifndef _FUNCTABLE_H
#define _FUNCTABLE_H

#include "elfobj.h"
#include "jump_target_list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct s_funcentry;
typedef struct s_funcentry FuncEntry;

struct s_funcentry {
	Elf32_Sym sym;
	JumpTargetList jtl;
	FuncEntry *copied;
	int dbg_presort_index;
};

void func_entry_init(FuncEntry *fe, Elf32_Sym *sym, int dbg_presort_index);

typedef struct {
	ElfObj *eo;
	FuncEntry *table;
	int capacity;
	int count;
} FuncTable;

void func_table_init(FuncTable *ft, ElfObj *eo);
void func_table_free(FuncTable *ft);

#ifdef __cplusplus
}
#endif

#endif // _FUNCTABLE_H
