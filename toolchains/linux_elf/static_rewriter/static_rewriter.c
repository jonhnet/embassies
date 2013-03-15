#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

#include "elfobj.h"
#include "elf_flat_reader.h"

#include "static_rewriter_args.h"
#include "add_segment.h"
#include "functable.h"
#include "string_list.h"
#include "disasm.h"
#include "hack_patch_init_tls.h"

bool scan_symbol(Args *args, ElfObj *eo, FuncEntry *fe, Disassembler *disasm)
{
	Elf32_Sym *sym = &fe->sym;

	disasm->num_funcs += 1;
	const char *symname = elfobj_get_string(eo, eo->strtable, sym->st_name);

	hack_patch_init_tls(eo, fe, disasm);

	bool patched;
	if ((disasm->num_funcs & args->dbg_conquer_mask)==0)
	{
		patched = false;
	}
	else if (args->dbg_conquer_match != 0xffffffff
		&& args->dbg_conquer_match != disasm->num_funcs)
	{
		patched = false;
	}
	else if (string_list_contains(&args->ignore_funcs, symname))
	{
		patched = false;
	}
	else
	{
		if (args->dbg_conquer_match != 0xffffffff
			|| args->dbg_conquer_mask != 0xffffffff)
		{
			printf("patching func %x: %s\n",
				disasm->num_funcs,
				elfobj_get_string(eo, eo->strtable, sym->st_name));
		}
		uint8_t *mapped_func = (uint8_t*)(eo->eri->elf_read_alloc)(eo->eri,
			elfobj_map_virtual_to_offset(eo, sym->st_value),
			sym->st_size);
		patched = patch_func(disasm,
			mapped_func,
			sym->st_size,
			sym->st_value,
			&fe->jtl,
			elfobj_get_string(eo, eo->strtable, sym->st_name));
	}
	
	if (patched)
	{
		disasm->num_with_int80s += 1;
	}
	return patched;
}

void scan_one_pass(ElfObj *eo, ElfFlatReader *efr, Args *args, char *dbg_infile, uint32_t *target_capacity)
{
	uint32_t target_offset = 0;
	uint32_t target_vaddr = 0;
	Elf32_Shdr *t_shdr = elfobj_find_shdr_by_name(eo, "target", NULL);
	if (t_shdr!=NULL)
	{
		target_offset = t_shdr->sh_offset;
		target_vaddr = t_shdr->sh_addr;
	}

	uint32_t pogo_vaddr = 0;
	Elf32_Shdr *p_shdr = elfobj_find_shdr_by_name(eo, "pogo", NULL);
	if (p_shdr!=NULL)
	{
		pogo_vaddr = p_shdr->sh_addr;
	}

	FuncTable ft;
	func_table_init(&ft, eo);

	// NB direct access to ElfFlatReader's storage. That's because we're
	// *updating it in place* before we write it out. So we need to
	// declare our pointer-to-bytes semantics, not the elf_read_alloc ifc's
	// (const) 'maybe give me a copy' semantics.
	Patcher patcher;
	patcher_init(&patcher, efr->bytes + target_offset, target_vaddr, pogo_vaddr, *target_capacity);

	Disassembler disasm;
	disasm_init(&disasm, &patcher);

	int i;
	for (i=0; i<ft.count; i++)
	{
		bool patched = scan_symbol(args, eo, &ft.table[i], &disasm);
		if (patched && args->dbg_symbols)
		{
			printf("[%s] Scanning %s\n",
				(*target_capacity==0) ? "measure" : "patch",
				elfobj_get_string(eo, eo->strtable, ft.table[i].sym.st_name));
			
		}
	}

	const char *dbg_pass_name;
	if (*target_capacity==0)
	{
		// return measurement info
		*target_capacity = patcher.next_target_space_offset;
		dbg_pass_name = "(measure pass)";
	}
	else
	{
		dbg_pass_name = "(patch pass)";
	}

	func_table_free(&ft);

	fprintf(stderr, "'%s' %s %d / %d funcs need rewriting\n",
		dbg_infile, dbg_pass_name, disasm.num_with_int80s, disasm.num_funcs);
}

// NB Another explicit ref to ElfFlatReader, since we're again
// modifying in place.
void hack_reloc_end(ElfObj *eo, ElfFlatReader *efr, char *reloc_end_file)
{
	Elf32_Sym *sym_end = elfobj_find_symbol(eo, "_end");
	Elf32_Sym *sym_GLOBAL_OFFSET_TABLE_ = elfobj_find_symbol(eo, "_GLOBAL_OFFSET_TABLE_");
	Elf32_Word page_mask = (1<<12)-1;
	uint32_t new_offset = ((sym_end->st_value + page_mask) & (~page_mask))
		- sym_GLOBAL_OFFSET_TABLE_->st_value;
	Elf32_Shdr *shdr = elfobj_find_shdr_by_name(eo, ".text", NULL);
	void *text = (void*) (efr->bytes + shdr->sh_offset);
	FILE *relocfp = fopen(reloc_end_file, "r");
	assert(relocfp!=NULL);
	int rc;
	while (1)
	{
		Elf32_Rel rel;
		rc = fread(&rel, sizeof(rel), 1, relocfp);
		if (rc!=1)
		{
			break;
		}
		((uint32_t*) (text + rel.r_offset))[0] = new_offset;
		printf("Wrote 0x%08x at 0x%08x\n", new_offset, (uint32_t) (rel.r_offset));
	}
	fclose(relocfp);
}

int main(int argc, char **argv)
{
	Args args;
	args_init(&args, argc, argv);

	// measure how much target space we need
	uint32_t target_capacity = 0;
	{
		ElfFlatReader efr;
		efr_read_file(&efr, args.input);

		ElfObj elfobj;
		elfobj_scan(&elfobj, &efr.ifc);
		scan_one_pass(&elfobj, &efr, &args, args.input, &target_capacity);
		elfobj_free(&elfobj);
	}

	// copy the file, inserting a suitable-sized segment
	char tmpfile[500];
	sprintf(tmpfile, "%s-tmp", args.output);

	add_segment(args.input,
		tmpfile,
		write_pogo(NULL),
		target_capacity,
		args.keep_new_phdrs_vaddr_aligned);

	int rc;
	rc = chmod(tmpfile, 0755);
	assert(rc==0);
	
	// now do the scan again, actually patching this time
	{
		ElfFlatReader efr;
		efr_read_file(&efr, tmpfile);

		ElfObj elfobj;
		elfobj_scan(&elfobj, &efr.ifc);
		scan_one_pass(&elfobj, &efr, &args, tmpfile, &target_capacity);

		Elf32_Shdr *p_shdr = elfobj_find_shdr_by_name(&elfobj, "pogo", NULL);
		write_pogo(efr.bytes+p_shdr->sh_offset);

		if (args.reloc_end_file!=NULL)
		{
			hack_reloc_end(&elfobj, &efr, args.reloc_end_file);
		}

		efr_write_file(&efr, tmpfile);
		elfobj_free(&elfobj);
	}

	rc = rename(tmpfile, args.output);
	if (rc!=0) { perror("rename"); }
	assert(rc==0);

	return 0;
}
