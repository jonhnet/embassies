#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include <elf.h>
#include "elfobj.h"
#include "elf_flat_reader.h"

char *pathmake(char *a, char *b)
{
	char *c = malloc(strlen(a)+strlen(b)+1);
	strcpy(c, a);
	strcat(c, b);
	return c;
}

int main(int argc, char **argv)
{
	assert(argc==5);
	char *libc_base = argv[1];
	char *osfile = pathmake(libc_base, argv[2]);
	char *linker_script = pathmake(libc_base, argv[3]);
	char *outfile = argv[4];
	char *mbr_in_cp = "mbr_in_cp.os";
	char *mbr_real_bss = "mbr_real_bss.os";

	unlink(mbr_in_cp);
	char cmd[5000];
	sprintf(cmd, "cp '%s' '%s'", osfile, mbr_in_cp);
	int rc = system(cmd);
	assert(rc==0);

	ElfObj elfobj, *eo=&elfobj;
	ElfFlatReader efr;
	efr_read_file(&efr, mbr_in_cp);
	elfobj_scan(eo, &efr.ifc);
	
	int si = elfobj_find_section_index_by_name(eo, ".bss");
	assert(si!=-1);
	ElfWS_Shdr *bss = &eo->shdr[si];

	bss->sh_offset = eo->eri->size;
	bss->sh_type = SHT_PROGBITS;

	//////////////////////////////////////////////////////////////////////
	// Relocate _end early.
	//////////////////////////////////////////////////////////////////////
	int _rel_text_si = elfobj_find_section_index_by_name(eo, ".rel.text");
	ElfWS_Shdr *reloc_shdr = &eo->shdr[_rel_text_si];
	ElfWS_Rel *relocs = (ElfWS_Rel *)(efr.ifc.elf_read_alloc)(&efr.ifc,
		reloc_shdr->sh_offset,
		reloc_shdr->sh_size);
		
	ElfWS_Sym *symtab = elfobj_fetch_symtab(eo);

	int num_relocs = (reloc_shdr->sh_size) / sizeof(Elf32_Rel);
	printf("I see %d reloc records\n", num_relocs);
	FILE *reloc_end_fp = fopen("reloc_end", "w");
	int reloc_i;
	for (reloc_i=0; reloc_i<num_relocs; reloc_i++)
	{
		int symidx = ELF32_R_SYM(relocs[reloc_i].r_info);
		ElfWS_Sym *symobj = &symtab[symidx];
		const char *symname = elfobj_get_string(eo, eo->strtable, symobj->st_name);
		if (strcmp(symname, "_end")==0)
		{
			printf("  ...here's an _end at %08x\n", (uint) relocs[reloc_i].r_offset);
			rc = fwrite(&relocs[reloc_i], sizeof(Elf32_Rel), 1, reloc_end_fp);
			assert(rc==1);
		}
	}
	fclose(reloc_end_fp);

	// write out the modified file, tacking on a bag of zeros
	// for bss to point to.
	FILE *ofp = fopen(mbr_real_bss, "w");

	rc = fwrite(efr.bytes, efr.ifc.size, 1, ofp);
	assert(rc==1);

	fflush(ofp);

	char *zeros = malloc(bss->sh_size);
	memset(zeros, 0, bss->sh_size);
	rc = fwrite(zeros, bss->sh_size, 1, ofp);
	assert(rc==1);

	fclose(ofp);

	sprintf(cmd,
//		"gcc-4.4"
		"gcc"
		" -nostdlib -nostartfiles -shared"
		" -o %s"
		" -Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both -Wl,-z,defs"
		" %s"
		" -Wl,--version-script=%sld.map"
		" -Wl,-soname=ld-linux.so.2 "
		"-T %s",
		outfile,
		mbr_real_bss,
		libc_base,
		linker_script);
	printf("executing: %s\n", cmd);
	system(cmd);

	return 0;
}
