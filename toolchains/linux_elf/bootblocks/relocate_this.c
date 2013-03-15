#include <elf.h>
#include "pal_abi/pal_types.h"
#include "LiteLib.h"
#include "elfobj.h"
#include "elf_flat_reader.h"

void relocate_this(uint32_t elfbase, uint32_t rel_offset, uint32_t rel_count)
{
	Elf32_Rel *rel0 = (Elf32_Rel *) (elfbase+rel_offset);

	ElfFlatReader efr;
	elf_flat_reader_init(&efr, (void*) elfbase, 0x7fffffff /*lies!*/, false);
	ElfObj elfobj, *eo=&elfobj;
	elfobj_scan(eo, &efr.ifc);
	ElfWS_Sym *symtab = elfobj_fetch_symtab(eo);

	// TODO: clean elfobj interface to symtabs.
	int num_syms = eo->symtab_section->sh_size/eo->symtab_section->sh_entsize;

	int ri;
	for (ri=0; ri<rel_count; ri++)
	{
		Elf32_Rel *relp = &rel0[ri];
		switch (ELF32_R_TYPE(relp->r_info))
		{
		case R_386_RELATIVE:
		{
			uint32_t *addr = (uint32_t*) (elfbase+relp->r_offset);
			(*addr) += elfbase;
			break;
		}
		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT:
			// NB readelf --relocs calls this a "R_386_JUMP_SLOT", with a U.
		case R_386_32:
		{
			int symidx = ELF32_R_SYM(relp->r_info);
			lite_assert(symidx < num_syms);
			ElfWS_Sym *symobj = &symtab[symidx];
			uint32_t *addr = (uint32_t*) (elfbase+relp->r_offset);
			
			// In a SHT_REL reloc section, such as we are iterating through,
			// we're supposed to add the symbol value to whatever value's
			// sitting in the location already.
			// We know we're reading a SHT_REL because that's where the
			// reloc offset came from, by way of the machine code preamble
			// SPLATted in by elf_flatten.c.
			// And we understand what SHT_REL vs SHT_RELA means from
			// eglibc-2.11.2/sysdeps/i386, where the symbol R_386_32 appears
			// twice. The _rela version ignores the existing value
			// and adds in an added from elsewhere. The _rel version
			// adds the existing value to the symbol value plus
			// sym_map->l_addr, which I seem to recall is the loaded
			// base of the elf.
			(*addr) += (elfbase + symobj->st_value);
		}
		case R_386_NONE:
			// this case is boring because it comes from a linker
			// over-allocating reloc spots. Safe to ignore.
			break;
		case R_386_PC32:
		{
			// no idea if I got this calculation right; we don't have
			// ane code that actually uses this path. (Just some linkage
			// that generates these relocs, so we have to do *something*
			// here.) Actually, let's put something conspicuous here.
			uint32_t *addr = (uint32_t*) (elfbase+relp->r_offset);
			(*addr) = 0x99999999;
			// (*addr) += (elfbase + symobj->st_value) - (addr-elfbase);
			break;
		}
		default:
			lite_assert(0);
		}
	}
}


