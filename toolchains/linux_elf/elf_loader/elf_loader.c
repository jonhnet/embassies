#include <stdio.h>
#include <string.h>
#include <elf.h>
#include <alloca.h>

#include "ElfLoad.h"
#include "ELDynamic.h"
#include "LiteLib.h"
#include "elfobj.h"
#include "elf_flat_reader.h"

#include "loader.h"
#include "ElfArgs.h"
#include "pal_abi/pal_abi.h"
#include "xinterpose.h"
#include "x86assembly.h"
#include "relocate_this.h"
#include "perf_measure.h"
#include "exception_test.h"

ELDynamic eld __attribute__ ((section (".data")));

void patch_pogo(ElfLoaded *el, xinterpose_f *xinterpose)
{
	ElfFlatReader efr;
	uint32_t size = 0x7fffffff;
		// we don't actually have any idea how big this thing is. Lie.
	elf_flat_reader_init(&efr, (void*) el->ehdr, size, false);

	ElfObj elfobj, *eo=&elfobj;
	elfobj_scan(eo, &efr.ifc);

	Elf32_Shdr *shdr = elfobj_find_shdr_by_name(eo, "pogo", NULL);
	char *code_ptr = (char *) efr.bytes + shdr->sh_offset;

	lite_memcpy(code_ptr, (char*) x86_jmp_relative, sizeof(x86_jmp_relative));
	code_ptr += sizeof(x86_jmp_relative);
	uint32_t jump_origin = ((uint32_t) code_ptr)+sizeof(uint32_t);
	uint32_t jump_offset = ((uint32_t) xinterpose) - jump_origin;
	lite_memcpy(code_ptr, (char*) &jump_offset, sizeof(uint32_t));

	elfobj_free(eo);
}

void elfmain(ZoogDispatchTable_v1 *zdt, uint32_t elfbase, uint32_t rel_offset, uint32_t rel_count)
{
	uint32_t top_of_stack;
	__asm__(
		"mov	(%%ebp),%%eax;"
		"mov	%%eax,%0;"
		: "=m"(top_of_stack)
		:	/* no inputs */
		: "%eax"	/* no clobbers */
		);

	// First things first: fix up relocs so we can use globals & func ptrs
	// and pretty much everything else.
	void _elfstart(void);

	relocate_this(elfbase, rel_offset, rel_count);

	ELDynamic *eld_local = &eld;
	(void) eld_local;
	char *loader_local = loader;
	ElfLoaded elf_dynloader = scan_elf(loader_local);

	perf_measure_mark_time(zdt, "start");

	// TODO no longer necessary to pass zdt through auxv; ld.so's not
	// going to be looking for it there.
	StartupContextFactory scf;
	uint32_t size_required = scf_count(&scf, eld.manifest_name, zdt);
	void *space = alloca(size_required);
	scf_fill(&scf, space);

//	perf_measure_mark_time(zdt, exception_test());

	xinterpose_init(zdt, top_of_stack, &scf);
	patch_pogo(&elf_dynloader, &xinterpose);

	void *startFunc = (void*) (elf_dynloader.ehdr->e_entry + elf_dynloader.displace);
	asm(
		"movl	%1,%%esp;"
		"movl	%0,%%eax;"
		"jmp	*%%eax;"
		:	/*output*/
		: "m"(startFunc), "m"(space)	/* input */
		: "%eax"	/* clobber */
	);
}
