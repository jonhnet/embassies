#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "elfobj.h"

#if ELFOBJ_USE_LIBC
#include <assert.h>

static inline int elfobj_strcmp(const char *a, const char *b)
{
	return strcmp(a, b);
}

#else // !ELFOBJ_USE_LIBC

#include "LiteLib.h"

static inline void assert(bool cond)
{
	lite_assert(cond);
}

static inline int elfobj_strcmp(const char *a, const char *b)
{
	return lite_strcmp(a, b);
}

#endif // ELFOBJ_USE_LIBC

bool elfobj_check_magic(ElfObj *eo)
{
	return (eo->ehdr->e_ident[EI_MAG0]==ELFMAG0
		&&	eo->ehdr->e_ident[EI_MAG1]==ELFMAG1
		&&	eo->ehdr->e_ident[EI_MAG2]==ELFMAG2
		&&	eo->ehdr->e_ident[EI_MAG3]==ELFMAG3);
}

bool elfobj_scan(ElfObj *eo, ElfReaderIfc *eri)
{
	memset(eo, -1, sizeof(*eo));

	eo->eri = eri;

	eo->ehdr = (eo->eri->elf_read_alloc)(eri, 0, sizeof(eo->ehdr[0]));

	if (eo->ehdr==NULL || !elfobj_check_magic(eo))
	{
		memset(eo, 0xe0, sizeof(*eo));
		return false;
	}

	eo->phdr = (ElfWS_Phdr*)(eo->eri->elf_read_alloc)(eri,
		eo->ehdr->e_phoff, sizeof(eo->phdr[0])*eo->ehdr->e_phnum);

	if (eo->ehdr->e_shnum > 0)
	{
		eo->shdr = (ElfWS_Shdr*)(eo->eri->elf_read_alloc)(eri,
			eo->ehdr->e_shoff, sizeof(eo->shdr[0])*eo->ehdr->e_shnum);
		eo->strtable = NULL;
		if (eo->ehdr->e_shstrndx < eo->ehdr->e_shnum)
		{
			eo->shstrtable = (const char *)(eo->eri->elf_read_alloc)(eri,
				eo->shdr[eo->ehdr->e_shstrndx].sh_offset,
				eo->shdr[eo->ehdr->e_shstrndx].sh_size);
		//	eo->shstrtab_sec = elfobj_find_section_by_type(eo, SHT_STRTAB);
		//	eo->shstrtable = (const char *) (eo->elfimage + eo->shstrtab_sec->sh_offset);
			ElfWS_Shdr *strtab_shdr = elfobj_find_shdr_by_name(eo, ".strtab", NULL);
			if (strtab_shdr!=NULL)
			{
				eo->strtable = (const char *)(eo->eri->elf_read_alloc)(eri,
					strtab_shdr->sh_offset,
					strtab_shdr->sh_size);
			}
		}

		eo->_symtab = NULL;
	}

	return true;
}

void elfobj_free(ElfObj *eo)
{
	(eo->eri->elf_dtor)(eo->eri);
	eo->eri = NULL;
}

ElfWS_Phdr *elfobj_find_segment_by_type(ElfObj *eo, ElfWS_Word type)
{
	int pi;
	for (pi=0; pi<eo->ehdr->e_phnum; pi++)
	{
		if (eo->phdr[pi].p_type == type)
		{
			return &eo->phdr[pi];
		}
	}
	return NULL;
}

ElfWS_Shdr *elfobj_find_shdr_by_type(ElfObj *eo, ElfWS_Word type)
{
	int si;
	for (si=0; si<eo->ehdr->e_shnum; si++)
	{
		if (eo->shdr[si].sh_type == type)
		{
			return &eo->shdr[si];
		}
	}
	return NULL;
}

const char *elfobj_get_string(ElfObj *eo, const char *strtable, ElfWS_Word sring_table_index)
{
	if (strtable==NULL)
	{
		return NULL;
	}
	return &strtable[sring_table_index];
}

int elfobj_find_section_index_by_name(ElfObj *eo, const char *name)
{
	int si;
	for (si=0; si<eo->ehdr->e_shnum; si++)
	{
		const char *namei =
			elfobj_get_string(eo, eo->shstrtable, eo->shdr[si].sh_name);
		if (elfobj_strcmp(namei, name)==0)
		{
			return si;
		}
	}
	return -1;
}

ElfWS_Shdr *elfobj_find_shdr_by_name(ElfObj *eo, const char *name, /* OUT OPTIONAL */ int *out_section_idx)
{
	int section_idx = elfobj_find_section_index_by_name(eo, name);
	if (section_idx ==-1)
	{
		return NULL;
	}

	if (out_section_idx!=NULL)
	{
		*out_section_idx = section_idx;
	}
	ElfWS_Shdr *shdr = &eo->shdr[section_idx];
	return shdr;
}

void *elfobj_read_section_by_name(ElfObj *eo, const char *name, /* OUT OPTIONAL */ int *out_section_idx)
{
	ElfWS_Shdr *shdr = elfobj_find_shdr_by_name(eo, name, out_section_idx);

	void *result = (eo->eri->elf_read_alloc)(
		eo->eri, shdr->sh_offset, shdr->sh_size);
	
	return result;
}

ElfWS_Off elfobj_map_virtual_to_offset(ElfObj *eo, ElfWS_Word virt)
{
	int pi;
	for (pi=0; pi<eo->ehdr->e_phnum; pi++)
	{
		ElfWS_Phdr *phdr = &eo->phdr[pi];
		ElfWS_Off begin = phdr->p_vaddr;
		ElfWS_Off end = phdr->p_vaddr + phdr->p_memsz;
		if (begin <= virt && virt < end)
		{
			ElfWS_Off memoffset = virt - begin;
			return memoffset;
		}
	}
	assert(false);	// virt not found in any segment
	return -1;
}

ElfWS_Sym *elfobj_fetch_symtab(ElfObj *eo)
{
	eo->symtab_section = elfobj_find_shdr_by_type(eo, SHT_SYMTAB);
	if (eo->symtab_section==NULL)
	{
		eo->symtab_section = elfobj_find_shdr_by_type(eo, SHT_DYNSYM);
	}
	assert(eo->symtab_section!=NULL);
	eo->_symtab = (ElfWS_Sym *)(eo->eri->elf_read_alloc)(eo->eri,
		eo->symtab_section->sh_offset,
		eo->symtab_section->sh_size);
	return eo->_symtab;
}

ElfWS_Sym *elfobj_find_symbol(ElfObj *eo, char *symname)
{
	elfobj_fetch_symtab(eo);

	int numsyms = eo->symtab_section->sh_size / sizeof(ElfWS_Sym);
	int si;
	for (si=0; si<numsyms; si++)
	{
		ElfWS_Sym *sym = &(eo->_symtab[si]);
		if (elfobj_strcmp(symname, elfobj_get_string(eo, eo->strtable, sym->st_name))==0)
		{
			return sym;
		}
	}
	return NULL;
}

void elfobj_scan_all_funcs(ElfObj *eo, scan_symbol_f *func, void *arg)
{
	elfobj_fetch_symtab(eo);

	int numsyms = eo->symtab_section->sh_size / sizeof(ElfWS_Sym);

/*
	fprintf(stderr, "Found a SHT_SYMTAB as %s with %d syms\n",
		elfobj_get_string(eo, eo->shstrtable, eo->symtab_section->sh_name), numsyms);
*/

	int si;
	for (si=0; si<numsyms; si++)
	{
		ElfWS_Sym *sym = &(eo->_symtab[si]);
		ElfWS_Word sttype = ELFWS_ST_TYPE(sym->st_info);
		if (sttype==STT_FUNC && sym->st_shndx!=SHN_UNDEF)
		{
			assert(sym->st_value!=0);
			(*func)(eo, sym, arg);
		}
	}
}

int _elfobj_get_phdrs_pass(ElfObj *eo, ElfWS_Word p_type, ElfWS_Phdr **ary)
{
	int i, count=0;
	for (i=0; i<eo->ehdr->e_phnum; i++)
	{
		if (eo->phdr[i].p_type == p_type)
		{
			if (ary!=NULL)
			{
				ary[count] = &eo->phdr[i];
			}
			count++;
		}
	}
	return count;
}

#if ELFOBJ_USE_LIBC
ElfWS_Phdr **elfobj_get_phdrs(ElfObj *eo, ElfWS_Word p_type, int *count)
{
	*count = _elfobj_get_phdrs_pass(eo, p_type, NULL);
	ElfWS_Phdr **ary = (ElfWS_Phdr **) malloc(sizeof(ElfWS_Phdr *)*(*count));
	_elfobj_get_phdrs_pass(eo, p_type, ary);
	return ary;
}
#endif
