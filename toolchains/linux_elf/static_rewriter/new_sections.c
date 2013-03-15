#include <string.h>

#include "new_sections.h"

void nss_init(NewSections *nss, void *mapped_address, uint32_t vaddr, uint32_t file_offset, uint32_t size, int num_new_sections, int extra_shstrtab_size)
{
	nss->size = size;
	nss->mapped_address = mapped_address;
	nss->vaddr = vaddr;
	nss->file_offset = file_offset;
	nss->next_allocation = 0;
	assert(num_new_sections <= MAX_SECTIONS);
	nss->num_new_sections = num_new_sections;
	nss->next_idx = 0;
	if (extra_shstrtab_size>0)
	{
		nss->extra_shstrtab_space = malloc(extra_shstrtab_size);
	} else {
		nss->extra_shstrtab_space = NULL;
	}
	nss->extra_shstrtab_size = extra_shstrtab_size;
	nss->extra_shstrtab_offset = 0;
}

NewSection *nss_alloc_section(NewSections *nss, char *name, uint32_t size)
{
	NewSection *ns = &nss->newsections[nss->next_idx];
	ns->idx = nss->next_idx;
	nss->next_idx+=1;
	assert(nss->next_idx <= MAX_SECTIONS);
	ns->size = size;
	if (nss->mapped_address == NULL)
	{
		// provide a fake place to work on the 'measure' pass
		// TODO We're just gonna leak this.
		ns->mapped_address = malloc(size);
	}
	else
	{
		ns->mapped_address = nss->mapped_address + nss->next_allocation;
	}
	ns->file_offset = nss->file_offset + nss->next_allocation;
	ns->vaddr = nss->vaddr + nss->next_allocation;
	nss->next_allocation += size;

	int name_len = strlen(name)+1;	// include terminating null
	ns->extra_shstrtab_name_idx = nss->extra_shstrtab_offset;
	if (nss->extra_shstrtab_space!=NULL)
	{
		ns->name = &nss->extra_shstrtab_space[nss->extra_shstrtab_offset];
		memcpy(ns->name, name, name_len);
	}
	else
	{
		ns->name = NULL;
	}
	nss->extra_shstrtab_offset += name_len;

	ns->sh_type = SHT_PROGBITS;

	return ns;
}

void nss_write_sections(NewSections *nss, NewSection *ns_shdrs, ElfObj *eo)
{
	Elf32_Shdr *newShdrs = (Elf32_Shdr *) ns_shdrs->mapped_address;
	memcpy(newShdrs, eo->shdr, sizeof(Elf32_Shdr)*eo->ehdr->e_shnum);
	Elf32_Shdr *lastShdr = &newShdrs[eo->ehdr->e_shnum];

	Elf32_Shdr *old_shstrtab = elfobj_find_shdr_by_name(eo, ".shstrtab", NULL);
	uint32_t old_shstrtab_size = old_shstrtab->sh_size;

	int i;
	for (i=0; i<nss->num_new_sections; i++)
	{
		NewSection *ns = &nss->newsections[i];
		lastShdr[i].sh_name = ns->extra_shstrtab_name_idx + old_shstrtab_size;
		lastShdr[i].sh_type = SHT_PROGBITS;
		lastShdr[i].sh_flags = SHF_ALLOC|SHF_EXECINSTR;
		lastShdr[i].sh_addr = ns->vaddr;
		lastShdr[i].sh_offset = ns->file_offset;
		lastShdr[i].sh_size = ns->size;
		lastShdr[i].sh_link = 0;
		lastShdr[i].sh_info = 0;
		lastShdr[i].sh_addralign = 1;
		lastShdr[i].sh_entsize = 0;
	}
}

