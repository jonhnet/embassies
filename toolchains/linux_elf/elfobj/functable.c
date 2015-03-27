#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "functable.h"

void func_entry_init(FuncEntry *fe, Elf32_Sym *sym, int dbg_presort_index)
{
	fe->sym = *sym;
	jump_target_list_init(&fe->jtl);
	fe->copied = NULL;
	fe->dbg_presort_index = dbg_presort_index;
}

uint32_t func_table_scan_overlaps(FuncTable *ft, FuncEntry *newtable);

void _func_table_scan_symbol(ElfObj *eo, Elf32_Sym *sym, void *arg)
{
	FuncTable *ft = (FuncTable *) arg;
	if (ft->table != NULL)
	{
		func_entry_init(&ft->table[ft->count], sym, ft->count);
	}
	ft->count += 1;
}

int _func_compar(const void *a, const void *b)
{
	FuncEntry *fa = (FuncEntry *) a;
	FuncEntry *fb = (FuncEntry *) b;
	int c0 = fa->sym.st_value - fb->sym.st_value;
	if (c0 != 0)
	{
		return c0;
	}
	int c1 = fa->sym.st_size - fb->sym.st_size;
	if (c1 != 0)
	{
		return c1;
	}
	// disambiguate deterministically; arbitrarily using symbol string order
	// (not lexically, just order in table)
	return fa->sym.st_name - fb->sym.st_name;
}

void _func_dump(FuncTable *ft, const char *desc)
{
#if 0
	int i;
	printf("DUMP %d symbols, %s:\n", ft->count, desc);
	for (i=0; i<ft->count; i++)
	{
		printf("  %3d: 0x%08x %6x %s\n",
			i,
			ft->table[i].sym.st_value,
			ft->table[i].sym.st_size,
			elfobj_get_string(ft->eo, ft->eo->strtable, ft->table[i].sym.st_name)
			);
	}
#endif
}

void func_table_init(FuncTable *ft, ElfObj *eo)
{
	ft->eo = eo;

	// count
	ft->table = NULL;
	ft->capacity = 0;
	ft->count = 0;
	elfobj_scan_all_funcs(eo, &_func_table_scan_symbol, ft);

	// alloc & store symbols
	ft->capacity = ft->count;
	ft->table = malloc(sizeof(FuncEntry)*ft->capacity);
	ft->count = 0;
	elfobj_scan_all_funcs(eo, &_func_table_scan_symbol, ft);

	assert(ft->count == ft->capacity);

	_func_dump(ft, "scanned");

	// sort to make the scan work
	qsort(ft->table, ft->count, sizeof(ft->table[0]), &_func_compar);

	{
		int i;
		for (i=1; i<ft->count; i++)
		{
			assert(ft->table[i-1].sym.st_value <= ft->table[i].sym.st_value);
		}
	}

	_func_dump(ft, "sorted");

	uint32_t new_capacity = func_table_scan_overlaps(ft, NULL);
	if (ft->capacity != new_capacity)
	{
//		printf("Reducing from %d to %d non-synonyms\n", ft->capacity, new_capacity);
	}
	FuncEntry *new_table = malloc(sizeof(FuncEntry)*new_capacity);
	uint32_t recount = func_table_scan_overlaps(ft, new_table);
	//printf("first: %d recount: %d\n", new_capacity, recount);
	assert(recount == new_capacity);
	free(ft->table);
	ft->table = new_table;
	ft->capacity = new_capacity;
	ft->count = new_capacity;
	_func_dump(ft, "reduced");
}

uint32_t func_table_scan_overlaps(FuncTable *ft, FuncEntry *newtable)
{
	ElfObj *eo = ft->eo;

	uint32_t uniques = 0;

	int i;
	for (i=0; i<ft->count; i++)
	{
		bool is_synonym = false;
		bool is_partial_synonym = false;

		if (i==0)
		{
			// first entry is automatically unique;
			// fall through to store it.
		}
		else
		{
			FuncEntry *fe0 = &ft->table[i-1];
			FuncEntry *fe1 = &ft->table[i];
			Elf32_Sym *s0 = &fe0->sym;
			Elf32_Sym *s1 = &fe1->sym;
			if (s0->st_value == s1->st_value)
			{
				// synonyms: two symbols point at the same entry point.
				// we expect that they're referring to the same whole
				// function; assert so.
				if (s0->st_size != s1->st_size)
				{
					printf("bummer, symbols %s and %s are synonyms with mismatching sizes.\n",
						elfobj_get_string(eo, eo->strtable, s0->st_name),
						elfobj_get_string(eo, eo->strtable, s1->st_name)
						);
					// let both symbols stand, and hope nothing breaks.
					assert(false); // We haven't see this in practice, so far.
				}
				else
				{
					is_synonym = true;
				}
			}
			else if (s0->st_value+s0->st_size > s1->st_value)
			{
				// s1 points into the middle of s0.
				if (s1->st_value+s1->st_size <= s0->st_value+s0->st_size)
				{
					// [glibc has this goofy pattern of _nocancels, e.g.
					// symbol waitpid is pointed into by __waitpid_nocancel's entry point]
					// In this case, s1 is a synonym for a subpiece of s0.
					is_partial_synonym = true;

					// We mark the target address as a jump_target.
					if (newtable!=NULL)
					{
					/*
						printf("%s makes a jump_target for %s\n",
							elfobj_get_string(eo, eo->strtable, s1->st_name),
							elfobj_get_string(eo, eo->strtable, s0->st_name)
							);
					*/

						// Write this result into the just-copied "unique"
						// thing that fe1 is poniting into.
						jump_target_list_add(&fe0->copied->jtl, s1->st_value - s0->st_value);
					}
				}
				else
				{
					// I've never seen such a case.
					printf("bummer, properly-overlapping symbols. %s is pointed into by %s's entry point\n",
						elfobj_get_string(eo, eo->strtable, s0->st_name),
						elfobj_get_string(eo, eo->strtable, s1->st_name)
						);
					assert(false);
				}
			}
		}

		if (newtable!=NULL)
		{
			if (is_partial_synonym)
			{
				ft->table[i].copied = NULL;
				// If a jump_target points into the middle of another
				// jump_target, we want things to explode hard.
			}
			else if (is_synonym)
			{
				ft->table[i].copied = ft->table[i-1].copied;
			}
			else
			{
				newtable[uniques] = ft->table[i];
				ft->table[i].copied = &newtable[uniques];
			}
		}
		if (!is_synonym && !is_partial_synonym)
		{
			uniques+=1;
		}
	}
	return uniques;
}

void func_table_free(FuncTable *ft)
{
	memset(ft->table, 17, sizeof(FuncEntry)*ft->capacity);
	free(ft->table);
}

