#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <stdbool.h>

#include "elfobj.h"
#include "elf_flat_reader.h"
#include "new_sections.h"

#define MIN(a,b)	(((a)<(b))?(a):(b))
#define MAX(a,b)	(((a)>(b))?(a):(b))

// In retrospect, sh_addr==NULL would have been a better test;
// NOBITS .bss and .tbss wouldn't have gotten confusing. we'd only
// have to special-case SHT_NULL (the first section).
bool file_alloced_bits(ElfWS_Shdr *shdr)
{
	return (shdr->sh_type!=SHT_NOBITS) && ((shdr->sh_flags & SHF_ALLOC)!=0);
	//return ((shdr->sh_flags & SHF_ALLOC)!=0);
}

typedef struct {
	ElfWS_Word start;
	ElfWS_Word end;
	bool first;
} Range;

void range_init(Range *range)
{
	range->first = true;
}

void range_grow(Range *range, ElfWS_Word start, ElfWS_Word size)
{
	ElfWS_Word end = start + size;
	if (range->first)
	{
		range->start = start;
		range->end = end;
		range->first = false;
	}
	else
	{
		range->start = MIN(range->start, start);
		range->end = MAX(range->end, end);
	}
}

void find_allocated_sections(ElfObj *eo, Range *offset_range, int *first_unallocated_section_idx)
{
	range_init(offset_range);

	int si;
	for (si=0; si<eo->ehdr->e_shnum; si++)
	{
		ElfWS_Shdr *sh = &eo->shdr[si];
		if (file_alloced_bits(sh))
		{
			range_grow(offset_range, sh->sh_offset, sh->sh_size);
		}
	}
#if DEBUG
	printf("sec_offset_range.start %08x\n", offset_range->start);
	printf("sec_offset_range.end   %08x\n", offset_range->end);
#endif

	int last_allocated_section_idx = 0;
	for (si=0; si<eo->ehdr->e_shnum; si++)
	{
		ElfWS_Shdr *sh = &eo->shdr[si];
		if (sh->sh_type==SHT_NOBITS)
		{
			// nothing to move around; doesn't matter where these end up.
			continue;
		}
		bool in_alloc_region =
			((sh->sh_offset >= offset_range->start)
			&& (sh->sh_offset+sh->sh_size <= offset_range->end));
		bool is_allocated = file_alloced_bits(sh);
		if (in_alloc_region != is_allocated)
		{
			assert(false);
		}
		if (is_allocated)
		{
			last_allocated_section_idx = si;
		}
	}

	// Now slurp in section 0:
	// the Ehdr, Phdr, and the mystery garbage stuck before section 1.
	// We don't want to move that.
	offset_range->start = 0;

	int bss_idx = elfobj_find_section_index_by_name(eo, ".bss");
	if (bss_idx == last_allocated_section_idx+1)
	{
		// don't shift .bss. It'll confuse something.
		last_allocated_section_idx = bss_idx;
	}

	if (first_unallocated_section_idx != NULL)
	{
		*first_unallocated_section_idx = last_allocated_section_idx+1;
	}
}

void find_load_segments(ElfObj *eo, Range *seg_offset_range, Range *seg_vaddr_range)
{
	// this should cover everything mapped by the original two phdrs.
	int load_phdr_count;
	ElfWS_Phdr **load_phdrs = elfobj_get_phdrs(eo, PT_LOAD, &load_phdr_count);

	range_init(seg_offset_range);
	range_init(seg_vaddr_range);
	int pi;
	for (pi=0; pi<load_phdr_count; pi++)
	{
		ElfWS_Phdr *ph = load_phdrs[pi];
		range_grow(seg_offset_range, ph->p_offset, ph->p_filesz);
		range_grow(seg_vaddr_range, ph->p_vaddr, ph->p_memsz);
	}
	free(load_phdrs);
}

void find_section_gaps(ElfObj *eo)
{
	int si;
	for (si=1; si<eo->ehdr->e_shnum; si++)
	{
		ElfWS_Shdr *s0 = &eo->shdr[si-1];
		ElfWS_Shdr *s1 = &eo->shdr[si];
		int gap = s1->sh_offset - (s0->sh_offset+s0->sh_size);
		if (gap != 0)
		{
			printf("Gap of 0x%4x bytes before section %d\n",
				gap,
				si);
		}
	}
}

void as_new_phdr(ElfObj *eo, NewSections *nss, ElfWS_Ehdr *newEhdr)
{
	NewSection *ns_phdrs = nss_alloc_section(nss, "new_phdrs", sizeof(ElfWS_Phdr)*(eo->ehdr->e_phnum+1));
	ElfWS_Phdr *newPhdrs = (ElfWS_Phdr*) ns_phdrs->mapped_address;
	memcpy(newPhdrs, eo->phdr, sizeof(ElfWS_Phdr)*eo->ehdr->e_phnum);
	ElfWS_Phdr *lastPhdr = &newPhdrs[eo->ehdr->e_phnum];
	lastPhdr->p_type = PT_LOAD;
	lastPhdr->p_offset = nss->file_offset;
	lastPhdr->p_vaddr = nss->vaddr;
	lastPhdr->p_paddr = nss->vaddr;
	lastPhdr->p_filesz = nss->size;
	lastPhdr->p_memsz = nss->size;
	lastPhdr->p_flags = PF_R|PF_X|PF_W;
	lastPhdr->p_align = 1<<12;

	// point Ehdr at new phdr
	newEhdr->e_phoff = ns_phdrs->file_offset;
	newEhdr->e_phnum = newEhdr->e_phnum+1;
}

NewSection *as_new_shstrtab_alloc(ElfObj *eo, NewSections *nss)
{
	int old_shstrtab_idx = elfobj_find_section_index_by_name(eo, ".shstrtab");
	assert(old_shstrtab_idx != -1);
	ElfWS_Shdr *old_shstrtab = &eo->shdr[old_shstrtab_idx];

	NewSection *ns = nss_alloc_section(nss, "new_shstrtab",
		old_shstrtab->sh_size+nss->extra_shstrtab_size);
	return ns;
}

void as_new_shstrtab_fill(ElfObj *eo, NewSections *nss, NewSection *ns, ElfWS_Ehdr *newEhdr)
{
	int old_shstrtab_idx = elfobj_find_section_index_by_name(eo, ".shstrtab");
	assert(old_shstrtab_idx != -1);
	ElfWS_Shdr *old_shstrtab = &eo->shdr[old_shstrtab_idx];

	(eo->eri->elf_read)(eo->eri, ns->mapped_address,
		old_shstrtab->sh_offset, old_shstrtab->sh_size);
	memcpy(
		ns->mapped_address+old_shstrtab->sh_size,
		nss->extra_shstrtab_space,
		nss->extra_shstrtab_size);

	// tell the Ehdr that this is the correct table to reference for
	// section header names.
	int new_shstrtab_idx = eo->ehdr->e_shnum+ns->idx;
	newEhdr->e_shstrndx = new_shstrtab_idx;
}

NewSection *as_new_shdr_alloc(ElfObj *eo, NewSections *nss)
{
	return nss_alloc_section(nss, "new_shdrs", sizeof(ElfWS_Shdr)*(eo->ehdr->e_shnum+nss->num_new_sections));
}

void as_new_shdr_fill(ElfObj *eo, NewSections *nss, NewSection *ns, ElfWS_Ehdr *newEhdr, int num_new_ections, uint32_t relocated_sections_offset)
{
	// Insert the updated shdr table...
	// This new section entry tells elf_loader/elfFlatten (TODO which looks at
	// sections, not segments -- maybe that's a mistake!) that the extended
	// segment needs to be included when folded into elf_loader.
	nss_write_sections(nss, ns, eo);

	// point Ehdr at new shdr
	newEhdr->e_shoff = ns->file_offset;
	newEhdr->e_shnum = newEhdr->e_shnum+nss->num_new_sections;
	
	ElfWS_Shdr *newShdrs = (ElfWS_Shdr *) ns->mapped_address;

	// Repair the relocated section headers
	ElfWS_Shdr *shdrs = newShdrs;

	Range sec_offset_range;
	int first_unallocated_section_idx;
	find_allocated_sections(eo, &sec_offset_range, &first_unallocated_section_idx);

	// We loop starting with the first unallocated section (because we don't
	// relocate any sections before that one), and stop at the end of the
	// original file sections. The new sections after that (written by
	// the call to nss_write_sections above) have correct offsets.

	int si;
	for (si=first_unallocated_section_idx; si<eo->ehdr->e_shnum; si++)
	{
		ElfWS_Shdr *sh = &shdrs[si];
		sh->sh_offset += relocated_sections_offset;
		//printf("shifting sec [%d] to %08x\n", si, sh->sh_offset);
	}
}

uint32_t as_new_sections(ElfObj *eo,
	void *mapped_address,
	uint32_t new_segment_vaddr,
	uint32_t new_segment_offset,
	uint32_t new_segment_size,	// not known until third pass
	uint32_t new_pogo_section_size,
	uint32_t new_target_section_size,
	uint32_t relocated_sections_offset,
	uint32_t *extra_shstrtab_size,
	ElfWS_Ehdr *newEhdr)
{
	NewSections nss_alloc, *nss=&nss_alloc;
	nss_init(nss, mapped_address, new_segment_vaddr, new_segment_offset, new_segment_size, 5, *extra_shstrtab_size);

	as_new_phdr(eo, nss, newEhdr);

	NewSection *ns_shdrs = as_new_shdr_alloc(eo, nss);

	NewSection *ns_shstrtab = as_new_shstrtab_alloc(eo, nss);
	(void) nss_alloc_section(nss, "pogo", new_pogo_section_size);
	(void) nss_alloc_section(nss, "target", new_target_section_size);

	as_new_shstrtab_fill(eo, nss, ns_shstrtab, newEhdr);
	as_new_shdr_fill(eo, nss, ns_shdrs, newEhdr, nss->num_new_sections, relocated_sections_offset);

	assert(nss->num_new_sections == nss->next_idx);
		// adjust num_new_sections parameter to match number of nss_alloc_sections calls

	if ((*extra_shstrtab_size)==0)
	{
		(*extra_shstrtab_size) = nss->extra_shstrtab_offset;
	}
	else
	{
		assert((*extra_shstrtab_size) == nss->extra_shstrtab_offset);
		assert((*extra_shstrtab_size) == nss->extra_shstrtab_size);
	}

	return nss->next_allocation;
}

void add_segment(char *infile, char *outfile,
	uint32_t new_pogo_section_size,
	uint32_t new_target_section_size,
	bool keep_new_phdrs_vaddr_aligned)
{
	ElfFlatReader efr;
	efr_read_file(&efr, infile);

	ElfObj eobj, *eo=&eobj;
	elfobj_scan(eo, &efr.ifc);
	
	assert(eo->ehdr->e_phentsize == sizeof(ElfWS_Phdr));

	//find_section_gaps(eo);

	Range sec_offset_range;
	find_allocated_sections(eo, &sec_offset_range, NULL);

	Range seg_offset_range;
	Range seg_vaddr_range;
	find_load_segments(eo, &seg_offset_range, &seg_vaddr_range);

#if DEBUG
	printf("seg_offset_start %08x\n", seg_offset_range.start);
	printf("seg_offset_end   %08x\n", seg_offset_range.end);
	printf("seg_vaddr_start %08x\n", seg_vaddr_range.start);
	printf("seg_vaddr_end   %08x\n", seg_vaddr_range.end);
#endif // DEBUG
	assert(sec_offset_range.start==seg_offset_range.start);
	assert(sec_offset_range.end==seg_offset_range.end);

	/* New file consists of:
	sections 0..last_alloced / load 0, load 1.
	new load 2, including these new sections:
		a rewritten program header (contiguous with rest of file, so offset
			matches vaddr)
		a rewritten section header (since we're adding sections)
		a rewritten sh_strtab (since we're adding section names)
		a pogo section (so loader can find & rewrite it)
		a target section (where all the static rewriting targets go)
	sections last_alloced+1..end.
	... this entails (a) rewriting section table to point to moved sections,
	and (b) knowing that the section table is one of the things we moved.
	*/
	int relocated_sections_size = efr.ifc.size - sec_offset_range.end;


	uint32_t page_mask = ((1<<12)-1);
	uint32_t new_segment_offset = (seg_offset_range.end + page_mask) & ~page_mask;
	uint32_t new_segment_vaddr = (seg_vaddr_range.end + page_mask) & ~page_mask;
#if DEBUG
	printf("new_segment_vaddr = 0x%08x\n", new_segment_vaddr);
#endif // DEBUG
	if (keep_new_phdrs_vaddr_aligned)
	{
		// don't expect to have to contract offset space; just expand it.
		assert(new_segment_offset <= new_segment_vaddr);
		// Be sure my idea of "a little smaller than" is sane; that we're
		// not wrapping the comparison.
		assert(new_segment_vaddr - new_segment_offset < 0x0040000);

		new_segment_offset = new_segment_vaddr;
	}
	uint32_t pad_size = new_segment_offset - seg_offset_range.end;


	ElfWS_Ehdr *fake_ehdr = malloc(sizeof(ElfWS_Ehdr));
		// give measurement pass something to poke at.

	// measure once to figure out how big new .shstrtab needs to be
	uint32_t extra_shstrtab_size = 0;
	as_new_sections(eo, NULL, new_segment_vaddr, new_segment_offset, new_pogo_section_size, -1, new_target_section_size, 0, &extra_shstrtab_size, fake_ehdr);

	// now that we know that, we measure again to figure out total
	// size of new section
	uint32_t new_segment_size =
		as_new_sections(eo, NULL, new_segment_vaddr, new_segment_offset, -1, new_pogo_section_size, new_target_section_size, 0, &extra_shstrtab_size, fake_ehdr);

	assert(new_segment_size > 0);
	uint32_t total_new_size =
		sec_offset_range.end + pad_size + new_segment_size + relocated_sections_size;

	// now we can actually allocate working space -- we know all the correct
	// sizes and offsets.
	void *new_elf = malloc(total_new_size);
	(efr.ifc.elf_read)(&efr.ifc, new_elf, 0, sec_offset_range.end);
	memset(new_elf+sec_offset_range.end, 0, pad_size);
	memset(new_elf+sec_offset_range.end+pad_size, 0, new_segment_size);
	(efr.ifc.elf_read)(&efr.ifc,
		new_elf+sec_offset_range.end+pad_size+new_segment_size,
		sec_offset_range.end,
		relocated_sections_size);

	uint32_t relocated_sections_offset =
		pad_size + new_segment_size;

	ElfWS_Ehdr *newEhdr = (ElfWS_Ehdr *) new_elf;

	as_new_sections(eo, new_elf+new_segment_offset, new_segment_vaddr, new_segment_offset, new_segment_size, new_pogo_section_size, new_target_section_size, relocated_sections_offset, &extra_shstrtab_size, newEhdr);

	int rc;
	FILE *ofp = fopen(outfile, "w");
	rc = fwrite(newEhdr, total_new_size, 1, ofp);
	assert(rc==1);
	fclose(ofp);

	free(new_elf);
}
