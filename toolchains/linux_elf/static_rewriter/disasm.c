#include <stdbool.h>
#include <malloc.h>
#include <alloca.h>
#include <string.h>
#include <libdis.h>
#include <assert.h>
#include <stdint.h>

#include "x86assembly.h"
#include "disasm.h"

void disasm_init(Disassembler *disasm, Patcher *patcher)
{
	disasm->patcher = patcher;
	x86_init(opt_none, NULL, NULL);
	disasm->num_funcs = 0;
	disasm->num_with_int80s = 0;
}

void _disasm_dbg_display_func(Disassembler *disasm, unsigned char *start, unsigned int len, uint32_t virt_addr, const char *prefix)
{
	int pos = 0;
	while (pos < len)
	{
		x86_insn_t insn;
		int insn_size = x86_disasm(start, len, 0, pos, &insn);
		assert(insn_size>0);
		char buf[500];
		x86_format_insn(&insn, buf, sizeof(buf), att_syntax);
		fprintf(stderr, "%s0x%08x %s\n", prefix, virt_addr+pos, buf);
		pos += insn_size;
	}
}

typedef struct {
	int pos;
	int size;
	bool bolted_down;
	uint32_t short_loop_candidate_target;
	bool jump_target;
		// doesn't need to be bolted, but can't be rewritten along
		// with a preceding instruction, because someone jumps to this
		// address. So this can be in a BlockState, but it must be
		// at the *start* of the BlockState.
	x86_insn_t insn;
} InstructionState;

void instruction_state_init(InstructionState *is, int pos)
{
	is->pos = pos;
	is->bolted_down = false;
	is->short_loop_candidate_target = 0;
	is->jump_target = false;
}

typedef struct {
	int insn_idx;
		// index into fs->instructions, pointing at first non-bolted
		// instruction of a contiguous set of non-bolted instructions.
	int insn_count;
		// number of instructions that live in this block.
	bool wants_rewrite;
		// this block contains an int 0x80
	int byte_count;
} BlockState;

void block_state_init(BlockState *bs, int insn_idx)
{
	bs->insn_idx = insn_idx;
	bs->insn_count = 0;
	bs->wants_rewrite = false;
	bs->byte_count = 0;
}

typedef struct {
	unsigned char *start;
	unsigned int len;
	uint32_t virt_addr;
	InstructionState *instructions;
	int insn_count;
	BlockState *blocks;
	int block_count;
	bool wants_rewrite;
} FuncState;

void func_state_init(FuncState *fs, unsigned char *start, unsigned int len, uint32_t virt_addr)
{
	fs->start = start;
	fs->len = len;
	fs->virt_addr = virt_addr;

	fs->instructions = (InstructionState*) malloc(len*sizeof(InstructionState));
		// can't have more instructions than we have bytes.
	fs->insn_count = 0;

	fs->blocks = (BlockState*) malloc(len*sizeof(BlockState));
		// same argument for basic blocks
	fs->block_count = 0;

	fs->wants_rewrite = false;
}

void func_state_free(FuncState *fs)
{
	free(fs->instructions);
	free(fs->blocks);
}

void func_state_create_instruction_table(FuncState *fs)
{
	int pos = 0;
	while (pos < fs->len)
	{
		InstructionState *is = &fs->instructions[fs->insn_count];
		instruction_state_init(is, pos);
		is->size = x86_disasm(fs->start, fs->len, fs->virt_addr, pos, &is->insn);
		assert(is->size > 0);
		fs->insn_count += 1;
		pos += is->size;
	}
}

int func_state_block_start_pos(FuncState *fs, BlockState *bs)
{
	return fs->instructions[bs->insn_idx].pos;
}

uint32_t _func_map_pos_to_insn_idx(FuncState *fs, int pos, bool *out_mid_jump)
{
	int insn_idx;
	for (insn_idx=0; insn_idx<fs->insn_count; insn_idx++)
	{
		InstructionState *is = &fs->instructions[insn_idx];
		if (pos == is->pos)
		{
			*out_mid_jump = false;
			return insn_idx;
		}
		if (pos < is->pos+is->size)
		{
			*out_mid_jump = true;
			return insn_idx;
		}
	}
	assert(false);	// couldn't find target instruction. Weird.
}
void _func_state_record_jump_target(FuncState *fs, int pos)
{
	bool mid_jump;
	int insn_idx = _func_map_pos_to_insn_idx(fs, pos, &mid_jump);
	InstructionState *is = &fs->instructions[insn_idx];
	if (!mid_jump)
	{
		is->jump_target = true;
	}
	else
	{
		// it's a jump *inside* an instruction. Yikes!
		// glibc has a penchant for jumping past a lock
		// prefix. x86 is one messed up architecture.
		// We need to bolt this instruction down to be sure
		// it can be jumped inside-of.
		is->bolted_down = true;
		// look for the lock prefix; I think that's the only time this
		// happens.
		assert(fs->start[is->pos] == 0xf0);
	}
}

void func_apply_external_jump_targets(FuncState *fs, JumpTargetList *jtl)
{
	int i;
	for (i=0; i<jtl->count; i++)
	{
		_func_state_record_jump_target(fs, jtl->targets[i]);
	}
}

uint32_t relative_jump_operand(x86_insn_t *insn, uint32_t base)
{
	x86_oplist_t *ol = insn->operands;
	uint32_t relative_operand = 0;
	while (ol!=NULL)
	{
		enum x86_op_type ot = ol->op.type;
		if (ot == op_relative_near || ot == op_relative_far)
		{
			// found a pc-relative reference
			assert(insn->group == insn_controlflow);	// else unimplemented
			uint32_t referenced_addr = 0;
			switch (ol->op.datatype)
			{
			case op_dword:
				assert((ol->op.flags & op_signed)!=0);
				referenced_addr = base + ol->op.data.sdword;
				break;
			case op_byte:
				referenced_addr = base + ol->op.data.sbyte;
				break;
			default:
				assert(false);	// unimplemented
			}
			assert (relative_operand == 0);	// only expecting one relative operand per insn
			relative_operand = referenced_addr;
		}
		ol = ol->next;
	}
	return relative_operand;
}

void func_state_find_block_boundaries(FuncState *fs)
{
	bool absorb_short_loops = true;
	int insn_idx;
	for (insn_idx=0; insn_idx<fs->insn_count; insn_idx++)
	{
		InstructionState *is = &fs->instructions[insn_idx];
		
		uint32_t referenced_addr = relative_jump_operand(&is->insn, fs->virt_addr + is->pos + is->size);

		if (is->insn.group == insn_controlflow)
		{
			switch (is->insn.type)
			{
			case insn_return:
				// that's okay; this can just move to target space and
				// return from there.
				break;
			case insn_call:
				// regardless of addressing mode, it may be a reloc;
				// if we move it, it won't get rewritten (and the loader
				// will splatter whatever we try to put here).
				is->bolted_down = true;
				break;
			case insn_jcc:
			case insn_jmp:
				// if this is relative and inside the function, it's
				// a basic block boundary.
				// Otherwise, we may have a reloc slot; can't move that,
				// either.
				if (absorb_short_loops
					&& referenced_addr >= fs->virt_addr
					&& referenced_addr < fs->virt_addr + fs->len)
				{
					assert(is->short_loop_candidate_target == 0);
					is->short_loop_candidate_target = referenced_addr;
				}
				else
				{
					is->bolted_down = true;
				}
				break;
			default:
				break;
			}
		}

		if (referenced_addr >= fs->virt_addr
			&& referenced_addr < fs->virt_addr+fs->len)
		{
			_func_state_record_jump_target(fs, referenced_addr-fs->virt_addr);
		}
	}
}

BlockState *_func_state_add_block(FuncState *fs, int insn_idx)
{
	BlockState *new_block = &fs->blocks[fs->block_count];
	fs->block_count += 1;
	block_state_init(new_block, insn_idx);
	return new_block;
}

bool block_contains_address(FuncState *fs, BlockState *block, uint32_t address)
{
	uint32_t block_start = fs->virt_addr + func_state_block_start_pos(fs, block);
	uint32_t block_end = block_start + block->byte_count;
	return (block_start<=address && address<block_end);
}

void func_state_find_blocks(FuncState *fs)
{
	BlockState *in_block = NULL;
	int insn_idx;
	for (insn_idx=0; insn_idx<fs->insn_count; insn_idx++)
	{
		InstructionState *is = &fs->instructions[insn_idx];
		if (is->short_loop_candidate_target != 0
			&& in_block
			&& !block_contains_address(
				fs, in_block, is->short_loop_candidate_target))
		{
			// There's an intra-function jump here that jumps out of the
			// block. Bolt it down.
			in_block = NULL;
			// Note that, since in_block is "growing", we don't know yet
			// what future instructions will be in it. Thus the test
			// conservatively accepts only backwards branches.
		}
		else if (is->bolted_down)
		{
			// close previous block, or stay closed.
			in_block = NULL;
			// ...and stay closed.
		}
		else if (is->jump_target)
		{
			// close any previous block,
			in_block = NULL;
			// and open a new one that this insn may belong to.
			// (we can move it and put a 'jmp' trampoline here,
			// since any inbound jump will find itself at the valid
			// beginning of the trampoline sequence).
			in_block = _func_state_add_block(fs, insn_idx);
		}
		else if (in_block == NULL)
		{
			// we should now be in a block, but we're not. fix that.
			in_block = _func_state_add_block(fs, insn_idx);
		}

		if (in_block!=NULL)
		{
			in_block->insn_count += 1;
			in_block->byte_count += is->size;
		}

		if (is->insn.type == insn_int)
		{
			assert(in_block!=NULL);
			in_block->wants_rewrite = true;
			fs->wants_rewrite = true;
		}
	}
}

void func_display_blocks(FuncState *fs, const char *symname)
{
	printf("\n*** Disassembly of %s(...)\n", symname);
	int block_num = 0;
	int insn_idx;
	for (insn_idx=0; insn_idx<fs->insn_count; insn_idx++)
	{
		InstructionState *is = &fs->instructions[insn_idx];
		char blockinfo[200];
		blockinfo[0] = '\0';

		BlockState *bs = &fs->blocks[block_num];
		while (true)
		{
			if (block_num >= fs->block_count)
			{
				// no more blockinfo
				break;
			}

			if (insn_idx >= bs->insn_idx+bs->insn_count)
			{
				// ooooh, ran off end of block. Fetch
				// next one before we decide if we're in it or not yet.
				block_num += 1;
				bs = &fs->blocks[block_num];
				// go back to top, because we may have just finished
				// the last block
				continue;
			}

			// okay, now we should either be before block_num
			// or in it.
			assert(insn_idx < bs->insn_idx+bs->insn_count);
			
			if (insn_idx >= bs->insn_idx)
			{
				sprintf(blockinfo, "b#%3d (%c %3dins %3d bytes)",
					block_num,
					bs->wants_rewrite ? 'R' : '_',
					bs->insn_count,
					bs->byte_count);
			}
			break;
		}

		char buf[500];
		x86_format_insn(&is->insn, buf, sizeof(buf), att_syntax);
		printf("%08x #%03d %27s %s\n",
			fs->virt_addr + is->pos, insn_idx, blockinfo, buf);
		if (is->short_loop_candidate_target!=0)
		{
			printf("    jumps to %08x\n", is->short_loop_candidate_target);
		}
	}
}

bool _patcher_patch_block(Patcher *patcher, FuncState *fs, BlockState *bs);

bool func_rewrite_blocks(FuncState *fs, Patcher *patcher)
{
	int block_idx;
	for (block_idx=0; block_idx < fs->block_count; block_idx++)
	{
		BlockState *bs = &fs->blocks[block_idx];
		if (bs->wants_rewrite)
		{
			InstructionState *is_first = &fs->instructions[bs->insn_idx];

			// double-check my accounting
			InstructionState *is_last = &fs->instructions[bs->insn_idx+bs->insn_count-1];
			assert(is_last->pos + is_last->size - is_first->pos == bs->byte_count);

			bool rc = _patcher_patch_block(patcher, fs, bs);
			if (!rc)
			{
				return false;
			}
		}
	}
	return true;
}

bool patch_func(Disassembler *disasm, unsigned char *start, unsigned int len, uint32_t virt_addr, JumpTargetList *external_jump_targets, const char *symname)
{
	// step 1: create a table of instructions
	// step 2: mark jump targets
	// step 3: create a table of basic blocks
	// step 4: mark blocks containing int 0x80s
	// step 5: patch those blocks out in their entirety.
	// which will work for every block as long as
	//		(a) it 's >= 5 bytes long and 
	// 		(b) it doesn't have the get_pc_thunk idiom in it.
	// (We can assert !(b) by looking for a call instruction.)
	FuncState fs;
	func_state_init(&fs, start, len, virt_addr);
	func_state_create_instruction_table(&fs);
	func_apply_external_jump_targets(&fs, external_jump_targets);
	func_state_find_block_boundaries(&fs);
	func_state_find_blocks(&fs);
	bool patched = fs.wants_rewrite;
	if (patched)
	{
		bool rc = func_rewrite_blocks(&fs, disasm->patcher);
		bool display = false;
		display |= (!rc);
//		display |= (strcmp(symname, "start_thread")==0);
		if (display)
		{
			func_display_blocks(&fs, symname);
		}
	}
	func_state_free(&fs);
	return patched;
}

// The "pogo" is the outbound trampoline -- the target code calls one
// common location to do int 0x80, and it gets rewritten to call a central
// syscall simulator. It has a different name to avoid confusion with
// the trampolines rewritten inline into the original binary code.
uint32_t write_pogo(uint8_t *pogo_space_mapped_base)
{
	int offset = 0;
	if (pogo_space_mapped_base!=NULL)
	{
		memcpy(
			pogo_space_mapped_base+offset,
			x86_int80,
			sizeof(x86_int80));
	}
	offset += sizeof(x86_int80);
	if (pogo_space_mapped_base!=NULL)
	{
		memcpy(
			pogo_space_mapped_base+offset,
			&x86_ret,
			sizeof(x86_ret));
	}
	offset += sizeof(x86_ret);

	// Toss in a little padding to give elf_loader room to rewrite the int x80
	// to a jump.
	// (Yeah. That's what I said. I'm rewriting that code.)
	int i;
	for (i=0; i<10; i++)
	{
		if (pogo_space_mapped_base!=NULL)
		{
			memcpy(pogo_space_mapped_base+offset, &x86_nop, sizeof(x86_nop));
		}
		offset += sizeof(x86_nop);
	}
	
	return offset;
}

void patcher_init(Patcher *patcher,
	uint8_t *target_space_mapped_base,
	uint32_t target_space_vaddr_base,
	uint32_t pogo_vaddr,
	uint32_t target_capacity)
{
	patcher->target_space_mapped_base = target_space_mapped_base;
	patcher->target_space_vaddr_base = target_space_vaddr_base;
	patcher->target_capacity = target_capacity;
	patcher->next_target_space_offset = 0;

	patcher->pogo_vaddr = pogo_vaddr;
}

void _patcher_update_branch(FuncState *fs, InstructionState *is,
	int32_t rel_address, uint8_t *dest, int curpos)
{
	x86_oplist_t *ol = is->insn.operands;
	assert(is->insn.group == insn_controlflow);	// else unimplemented
	while (ol!=NULL)
	{
		enum x86_op_type ot = ol->op.type;
		if (ot == op_relative_near || ot == op_relative_far)
		{
			break;
		}
		ol = ol->next;
	}

	// copy the instruction that we assume (assert) is 1 byte.
	dest[curpos] = ((uint8_t*) (fs->start + is->pos))[0];
	curpos+=1;

	// found a pc-relative reference
	switch (ol->op.datatype)
	{
	case op_dword:
		assert(is->insn.size==5);
		((int32_t*)(&dest[curpos]))[0] = rel_address;
		break;
	case op_byte:
		assert(is->insn.size==2);
		((int8_t*)(&dest[curpos]))[0] = (int8_t) rel_address;
		break;
	default:
		assert(false);	// unimplemented
	}
}

void _patcher_make_target(Patcher *patcher, FuncState *fs, BlockState *bs)
{
	int *original_block_idx_to_patched_block_pos_map =
		alloca(sizeof(uint32_t)*bs->insn_count);
	memset(original_block_idx_to_patched_block_pos_map, 0x17,
		sizeof(uint32_t)*bs->insn_count);
	uint8_t *dest = (patcher->target_capacity!=0)
		?  patcher->target_space_mapped_base + patcher->next_target_space_offset
		: NULL;

	int curpos = 0;
	int ii;
	for (ii=0; ii<bs->insn_count; ii++)
	{
		original_block_idx_to_patched_block_pos_map[ii] = curpos;
		InstructionState *is = &fs->instructions[bs->insn_idx + ii];
		if (is->insn.type == insn_int)
		{
			if (dest!=NULL)
			{
				memcpy(&dest[curpos], &x86_call, sizeof(x86_call));
				uint32_t vaddr_after_call =
					  patcher->target_space_vaddr_base
					+ patcher->next_target_space_offset
					+ curpos
					+ call_dispatcher_size;
				int32_t rel_address = patcher->pogo_vaddr
					- vaddr_after_call;
				memcpy(&dest[curpos]+sizeof(x86_call),
					&rel_address, sizeof(rel_address));
			}
			curpos += call_dispatcher_size;
		}
		else if (is->insn.group == insn_controlflow
			&& (is->insn.type==insn_jcc
				|| is->insn.type==insn_jmp))
		{
			bool mid_jump;
			uint32_t jump_target_pos = relative_jump_operand(&is->insn, is->pos+is->size);
			int jump_target_block_idx = _func_map_pos_to_insn_idx(fs, jump_target_pos, &mid_jump) - bs->insn_idx;
			assert(!mid_jump);	// should have been bolted down.
			assert(jump_target_block_idx >= 0 && jump_target_block_idx < ii);
				// else haven't actually filled in the array.
			int patched_target_block_pos =
				original_block_idx_to_patched_block_pos_map[jump_target_block_idx];
			if (dest!=NULL)
			{
				uint32_t pos_after_jump = curpos + is->size;
				int32_t rel_address = patched_target_block_pos - pos_after_jump;
				_patcher_update_branch(fs, is, rel_address, dest, curpos);
			}
			curpos += is->size;
		}
		else
		{
			if (dest!=NULL)
			{
				memcpy(&dest[curpos], fs->start + is->pos, is->size);
			}
			curpos += is->size;
		}
	}

	// add a jump back to trampoline site
	if (dest!=NULL)
	{
		memcpy(&dest[curpos], x86_jmp_relative, sizeof(x86_jmp_relative));
		uint32_t pos_after_target =
			patcher->target_space_vaddr_base
			+ patcher->next_target_space_offset
			+ curpos
			+ return_patch_size;
		uint32_t trampoline_return_point =
			fs->virt_addr + func_state_block_start_pos(fs, bs) + bs->byte_count;
		int32_t jump_return_offset = trampoline_return_point - pos_after_target;
		memcpy(
			&dest[curpos]+sizeof(x86_jmp_relative),
			&jump_return_offset,
			sizeof(jump_return_offset));
	}
	curpos += return_patch_size;

	patcher->next_target_space_offset += curpos;
}

void _patcher_make_trampoline(Patcher *patcher, FuncState *fs, BlockState *bs, uint32_t target_vaddr)
{
	uint8_t *trampoline_start = fs->start+func_state_block_start_pos(fs, bs);
	uint32_t trampoline_start_vaddr =
		fs->virt_addr+func_state_block_start_pos(fs, bs);

	if (patcher->target_capacity==0)
	{
		// this is just a measurement pass.
		// nothing to measure for the trampoline.
		return;
	}

	// patch in the trampoline

	assert(bs->byte_count >= trampoline_patch_size);

	// (We don't use 'call' because we don't want to torque the stack
	// pointer, lest some of the moved instructions make assumptions about
	// stack offsets. And we know the target "return address", anyway,
	// and that it's unique.)
	int trampoline_byte = 0;
	memcpy(&trampoline_start[trampoline_byte], x86_jmp_relative, sizeof(x86_jmp_relative));
	trampoline_byte+=sizeof(x86_jmp_relative);
	int32_t jump_out_offset =
		target_vaddr
		- (trampoline_start_vaddr + trampoline_patch_size);
	memcpy(&trampoline_start[trampoline_byte], &jump_out_offset, sizeof(jump_out_offset));
	trampoline_byte+=sizeof(jump_out_offset);
	for (; trampoline_byte<bs->byte_count; trampoline_byte++)
	{
		// these should never get accessed, so let's assert that
		// by putting 'hlt' here instead of 'nop'!
		trampoline_start[trampoline_byte] = x86_hlt;
	}
}

bool _patcher_patch_block(Patcher *patcher, FuncState *fs, BlockState *bs)
{
	// save this here -- its input get changed by make_target,
	// but we need it for make_trampoline.
	uint32_t target_vaddr =
		patcher->target_space_vaddr_base
		+ patcher->next_target_space_offset;

	// This computes the target size, whether or not it actually writes,
	// so we can measure first, then allocate a big-enough segment, and
	// come back to generate the actual instruction bytes.
	_patcher_make_target(patcher, fs, bs);

	assert(patcher->target_capacity==0
		|| patcher->next_target_space_offset <= patcher->target_capacity);

	if (bs->byte_count < trampoline_patch_size)
	{
		return false;
	}

	_patcher_make_trampoline(patcher, fs, bs, target_vaddr);

	return true;
}

