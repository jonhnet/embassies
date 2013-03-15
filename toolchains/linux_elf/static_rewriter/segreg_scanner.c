#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <libdis.h>

#include "elfobj.h"
#include "elf_flat_reader.h"

#include "segreg_args.h"
#include "add_segment.h"
#include "functable.h"
#include "string_list.h"
#include "disasm.h"

const char REPZ_XCRYPT_ECB[] = {0xf3, 0x0f, 0xa7, 0xc8};
const char XSTORE_RNG[] = {0x0f, 0xa7, 0xc0};
typedef struct {
	int len;
	const char *bytes;
} WeirdInstruction;
WeirdInstruction weird_instructions[] = {
	{sizeof(REPZ_XCRYPT_ECB), REPZ_XCRYPT_ECB},
	{sizeof(XSTORE_RNG), XSTORE_RNG},
	{0, NULL}
};

void scan_symbol(Args *args, ElfObj *eo, FuncEntry *fe)
{
	Elf32_Sym *sym = &fe->sym;

	uint32_t vaddr = sym->st_value;
	uint32_t len = sym->st_size;
	Elf32_Off offset = elfobj_map_virtual_to_offset(eo, vaddr);
	uint8_t* mapped = (eo->eri->elf_read_alloc)(eo->eri, offset, len);
	const char *symname = elfobj_get_string(eo, eo->strtable, sym->st_name);

	int segreg_posns[700];
	int num_segreg_posns = 0;

	int pos = 0;
	while (pos < len)
	{
		x86_insn_t insn;
		int size = x86_disasm(mapped, len, vaddr, pos, &insn);

		if (size==0)
		{
			int wi;
			for (wi=0; weird_instructions[wi].len>0; wi++)
			{
				if (pos+weird_instructions[wi].len <= len
					&& memcmp(
						mapped+pos,
						weird_instructions[wi].bytes,
						weird_instructions[wi].len)==0)
				{
					// x86_disasm doesn't know about this (weird VIA encryption
					// optimization) instruction. But we don't care about it,
					// so just skip on past. (insn not filled in, so skip over that
					// code).
					size = weird_instructions[wi].len;
					goto skip_undisassembled_instruction;
				}
			}
		}

		x86_oplist_t *oplist;
		for (oplist = insn.operands; oplist!=NULL; oplist=oplist->next)
		{
			bool mark = false;

			bool is_selector_access =
				(oplist->op.type==op_register &&
					oplist->op.data.reg.type == reg_seg);
			if (args->display_selector_accesses && is_selector_access)
			{
				mark = true;
			}

			int seg = (oplist->op.flags & 0xf00);
			if (args->display_seg_derefs && (seg==op_fs_seg || seg==op_gs_seg))
			{
				mark = true;
			}

			if (mark)
			{
				segreg_posns[num_segreg_posns++] = pos;
				//fprintf(stdout, "@%d flags 0x%02x\n", pos, oplist->op.flags);
			}
		}

skip_undisassembled_instruction:
		if (size==0)
		{
			if (strcmp(symname,"_fini")==0)
			{
				// _fini has incorrectly-labeled symbol size
				break;
			}

			fprintf(stdout, "Assert for filename %s, symbol %s, pos %d\n",
				args->input, symname, pos);
			assert(false);
		}
		pos += size;
	}

	if (num_segreg_posns>0)
	{
		fprintf(stdout, "file %s symbol %s has seg refs at...\n", args->input, symname);
		int spi;
		for (spi=0; spi<num_segreg_posns; spi++)
		{
			int pos = segreg_posns[spi];
			x86_insn_t insn;
			x86_disasm(mapped, len, vaddr, pos, &insn);
			char formatted_insn[80];
			x86_format_insn(
				&insn, formatted_insn, sizeof(formatted_insn), att_syntax);
			fprintf(stdout, "+%d  %s\n",
				pos,
				formatted_insn
				);
		}
	}
}

void scan_one_pass(Args *args, char *infile, uint32_t *target_capacity)
{
	ElfFlatReader efr;
	efr_read_file(&efr, infile);

	ElfObj elfobj, *eo=&elfobj;
	elfobj_scan(&elfobj, &efr.ifc);

	FuncTable ft;
	func_table_init(&ft, eo);

	int i;
	for (i=0; i<ft.count; i++)
	{
		scan_symbol(args, eo, &ft.table[i]);
	}

	func_table_free(&ft);
	elfobj_free(&elfobj);
}



int main(int argc, char **argv)
{
	Args args;
	args_init(&args, argc, argv);

	uint32_t target_capacity = 0;
	scan_one_pass(&args, args.input, &target_capacity);

	return 0;
}
