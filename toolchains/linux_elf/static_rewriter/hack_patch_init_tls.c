#include <string.h>
#include <assert.h>
#include <elf.h>
#include <libdis.h>

#include "x86assembly.h"
#include "hack_patch_init_tls.h"

// The thing we're patching is called from rtld.c:783 as TLS_INIT_TP,
// implemented in nptl/sysdeps/i386/tls.h; esp lines 217 (ask -1) and
// 248 (frob result <<3+3).

void hack_patch_init_tls(ElfObj *eo, FuncEntry *fe, Disassembler *disasm)
{
	Elf32_Sym *sym = &fe->sym;
	const char *symname = elfobj_get_string(eo, eo->strtable, sym->st_name);

	if (strcmp(symname, "init_tls")!=0)
	{
		return;
	}

	uint32_t vaddr = sym->st_value;
	Elf32_Off offset = elfobj_map_virtual_to_offset(eo, vaddr);
	uint32_t len = sym->st_size;
	uint8_t *mapped = (uint8_t*)
		(eo->eri->elf_read_alloc)(eo->eri, offset, len);

	int pos = 0;
	while (pos < len)
	{
		x86_insn_t insn;
		int size = x86_disasm(mapped, len, vaddr, pos, &insn);

		// change 'mov -1,blahblah' to -2, to signal to xax_posix_emulation
		// that we expect a gs descriptor, not an entry number, back in
		// the user_desc entry_number field.
		if (
			   insn.type==insn_mov
			&& insn.operand_count==2
			&& insn.operands->next[0].op.type==op_immediate
			&& insn.operands->next[0].op.datatype==op_dword
			&& insn.operands->next[0].op.data.sdword==-1
			&& insn.size == 7)
		{
			((int32_t *)(mapped+pos+3))[0] = -2;
		}

		// change from eax=(eax*8)|3 to nops.
		if (
			   insn.type==insn_mov
			&& insn.operand_count==2
			&& insn.operands[0].op.type==op_register
			&& strcmp(insn.operands[0].op.data.reg.name, "eax")==0
			&& insn.operands->next[0].op.type==op_expression
			&& insn.operands->next[0].op.data.expression.scale == 8
			&& insn.operands->next[0].op.data.expression.disp == 3
			&& strcmp(insn.operands->next[0].op.data.expression.index.name, "eax")==0)
		{
			memset(mapped+pos, x86_nop, size);
		}

#if 0
		// redo the disassembly so the format&print is correct
		size = x86_disasm(mapped, len, vaddr, pos, &insn);

		char buf[800];
		x86_format_insn(&insn, buf, sizeof(buf), att_syntax);
		fprintf(stdout, "@+%3d %s\n", pos, buf);
#endif

		assert(size!=0);
		pos += size;
	}
}

