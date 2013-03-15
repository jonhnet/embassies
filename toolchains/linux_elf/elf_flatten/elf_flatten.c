#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "ELDynamic.h"
#include "x86assembly.h"
#include "elf_flatten_args.h"

#include <stddef.h>		// offsetof
#include "pal_abi/pal_abi.h"

#define DBG_VERBOSE 0

typedef struct s_elf_state
{
	void *elfbase;
	Elf32_Ehdr *ehdr;
	size_t size;
} ElfState;

#define MAX_SECTIONS 200
typedef struct {
	Elf32_Addr min_address;
	Elf32_Addr past_max_address;
	int phase;
	int section_count;
	Elf32_Shdr *section_table;
	int section_remap_table[MAX_SECTIONS];
} SlurpState;

typedef struct s_binary_image
{
	SlurpState slurpState;
	char *start;
	Elf32_Off jump_shift;	// prefix space reserved for a jump-to-entry-point
	Elf32_Off size;
} BinaryImage;


int mapfile(const char *image, ElfState *es)
{
	struct stat sbuf;
	int rc = stat(image, &sbuf);
	if (rc!=0)
	{
		return -1;
	}
	es->size = sbuf.st_size;
	int fd = open(image, O_RDONLY, 0);
	if (fd<0)
	{
		return -1;
	}
	void *addr = mmap(0, es->size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr==MAP_FAILED)
	{
		perror("mmap");
		return -1;
	}
	es->elfbase = addr;
	es->ehdr = (Elf32_Ehdr *) addr;
	return 0;
}

int findSection(ElfState *elf, char *secName, Elf32_Shdr **out_secHdr)
{
	Elf32_Shdr *secHdr;
	int secCount;

	for (secCount = 0; secCount < elf->ehdr->e_shnum; secCount += 1)
	{
		secHdr = (Elf32_Shdr *) (elf->elfbase+elf->ehdr->e_shoff+secCount*elf->ehdr->e_shentsize);

		Elf32_Shdr *strSection = (Elf32_Shdr *)
			(elf->elfbase+elf->ehdr->e_shoff+elf->ehdr->e_shstrndx*elf->ehdr->e_shentsize);
		char *symbolName = (char *)
			(elf->elfbase + strSection->sh_offset + secHdr->sh_name);
		if (strcmp(symbolName, secName)==0)
		{
			*out_secHdr = secHdr;
			return 0;
		}
	}
	fprintf(stderr, "elfFlatten: no %s section\n", secName);
	return -1;
}

const char *elf_string(void *elfmapped, Elf32_Half shndx, int index)
{
	if (shndx==0) { return ""; }

	Elf32_Ehdr *elf = (Elf32_Ehdr *) elfmapped;
	if (shndx >= elf->e_shnum)
	{
		return "funnnnkay";
	}

	Elf32_Shdr *strsh = (Elf32_Shdr*)
		(elfmapped+elf->e_shoff+shndx*elf->e_shentsize);
	const char *strtable = (const char*)
		(elfmapped+strsh->sh_offset+index);
	return strtable;
}

uint32_t get_data_origin(ElfState *elf)
{
	Elf32_Shdr *data_sechdr;
	if (findSection(elf, ".data", &data_sechdr)!=0) { assert(0); }
	return data_sechdr->sh_addr - data_sechdr->sh_offset;
}

uint32_t et_get_symbol_base(void *elfmapped)
{
	Elf32_Ehdr *elf = (Elf32_Ehdr *) elfmapped;

	int i;
	for (i=0; i<elf->e_phnum; i++)
	{
		Elf32_Phdr *ph = (Elf32_Phdr *)
			(elfmapped + elf->e_phoff + i*elf->e_phentsize);
		if (ph->p_type == PT_LOAD
			&& (ph->p_flags & PF_X)!=0)
		{
			// found the text segment.
			return ph->p_vaddr;
		}
	}
	assert(0);
}

int elf_strtab(void *elfmapped)
{
	Elf32_Ehdr *elf = (Elf32_Ehdr *) elfmapped;

	int shi;
	for (shi=0; shi<elf->e_shnum; shi++)
	{
		Elf32_Shdr *sh = (Elf32_Shdr*)
			(elfmapped+elf->e_shoff+shi*elf->e_shentsize);
		if (strcmp(
			".strtab",
			elf_string(elfmapped, elf->e_shstrndx, sh->sh_name)
			)==0)
		{
			return shi;
			//return (char*) (elfmapped + sh->sh_offset);
		}
	}
	return -1;
}

int find_symbol(ElfState *elf, char *target_symname, int *out_sym_size)
{
	Elf32_Ehdr *ehdr = elf->ehdr;
	//uint32_t symbase = et_get_symbol_base(elf->elfbase);

	int strtab = elf_strtab(elf->elfbase);
	if (strtab == -1)
	{
		return 0;
	}

	// find symbol header
	int shi;
	for (shi=0; shi<ehdr->e_shnum; shi++)
	{
		Elf32_Shdr *sh = (Elf32_Shdr*)
			(elf->elfbase+ehdr->e_shoff+shi*ehdr->e_shentsize);
//		fprintf(stderr, "elfFlatten: Found a section header: %d %s\n", shi, elf_string(elf->elfbase, ehdr->e_shstrndx, sh->sh_name));
		if (sh->sh_type == SHT_SYMTAB)
		{
//			fprintf(stderr, "elfFlatten: Found a symtab: %s\n", elf_string(elf->elfbase, ehdr->e_shstrndx, sh->sh_name));
			break;
		}
	}
	assert(shi<ehdr->e_shnum);	// no symtable

	Elf32_Shdr *sh = (Elf32_Shdr*)
		(elf->elfbase+ehdr->e_shoff+shi*ehdr->e_shentsize);
	int i;
	int len = sh->sh_size / sh->sh_entsize;
	int count = 0;
	for (i = 0; i < len; i++)
	{
		Elf32_Sym *sym = (Elf32_Sym*) (elf->elfbase+sh->sh_offset+i*sh->sh_entsize);
		if (sym->st_name!=0)
		{
			const char *symname = elf_string(elf->elfbase, strtab, sym->st_name);
			if (strcmp(symname, target_symname)==0)
			{
				//uint32_t sym_origin = get_data_origin(elf);
				uint32_t symvalue = sym->st_value;
				fprintf(stderr, "elfFlatten: %s == %08x sz %d\n", symname, symvalue, sym->st_size);
				*out_sym_size = sym->st_size;
				return symvalue;
			}
			//store[count].size = sym->st_size;
			count += 1;
		}
	}
	assert(0);
}

void slurp_section(ElfState *es, BinaryImage *bi, Elf32_Shdr *shdr, int orig_shdr_idx)
{
	const char *disposition = "undefined";
	SlurpState *slurpState = &bi->slurpState;
	if ((shdr->sh_flags & SHF_ALLOC)!=0
		&& shdr->sh_type!=SHT_NOTE)
	{
		if (slurpState->phase==0)
		{
			if (shdr->sh_addr < slurpState->min_address)
			{
				slurpState->min_address = shdr->sh_addr;
			}
			Elf32_Addr pma = shdr->sh_addr+shdr->sh_size;
			if (pma > slurpState->past_max_address)
			{
				slurpState->past_max_address = pma;
			}
			assert(slurpState->section_count < MAX_SECTIONS);
#if DBG_VERBOSE
			fprintf(stderr, "section_remap_table[%d] = %d\n",
				orig_shdr_idx,
				slurpState->section_count);
#endif // DBG_VERBOSE
			slurpState->section_remap_table[orig_shdr_idx] = slurpState->section_count;
		}
		else if (slurpState->phase==1)
		{
			// Don't try to copy a section that doesn't have
			// bits (e.g. .bss). Still want to scan it, though,
			// to reserve space for it in the file image. Yes,
			// we're transmitting zeros for the empty space in
			// the boot block, because it's the simplest
			// specification. Want ELF? Load that next. :v)
			if (shdr->sh_type!=SHT_NOBITS)
			{
				Elf32_Addr offset = shdr->sh_addr - slurpState->min_address;
				memcpy(bi->start+offset+bi->jump_shift, es->elfbase+shdr->sh_offset, shdr->sh_size);
			}

			// build a section table, if asked to
			if (slurpState->section_table!=NULL)
			{
				memcpy(
					&slurpState->section_table[slurpState->section_count],
					shdr,
					sizeof(*shdr));
			}
		}
		else
		{
			assert(0);
		}
		disposition = "included";
		slurpState->section_count += 1;
	}
	else
	{
		disposition = "ignored";
	}
#if DBG_VERBOSE
	fprintf(stderr, "section [%d] type %d offset 0x%6x disposition %s alloc %d note %d\n",
		slurpState->section_count, shdr->sh_type, shdr->sh_offset, disposition,
		(shdr->sh_flags & SHF_ALLOC)!=0,
		shdr->sh_type!=SHT_NOTE);
#else
	(void) disposition;
#endif // DBG_VERBOSE
}

void lookup_rel_section(ElfState *es, uint32_t *out_rel_offset, uint32_t *out_rel_count, bool assert_rel_exists)
{
	int shcount;
	for (shcount = 0; shcount < es->ehdr->e_shnum; shcount += 1)
	{
		Elf32_Shdr *shdr = (Elf32_Shdr*)
			(es->elfbase + es->ehdr->e_shoff + shcount*es->ehdr->e_shentsize);
		if (shdr->sh_type == SHT_REL)
		{
			(*out_rel_offset) = shdr->sh_addr;
			(*out_rel_count) = shdr->sh_size / sizeof(Elf32_Rel);
			return;
		}
	}

	fprintf(stderr, "warning, no SHT_REL!\n");
	if (assert_rel_exists)
	{
		assert(0);
	}
	*out_rel_offset = 0;
	*out_rel_count = 0;
}

typedef struct {
	char jump_region[0x40];
	char dbg_elf_path[0x100];
} RawPrefix;

void install_entry_jump(ElfState *es, BinaryImage *bi, bool assert_rel_exists, uint32_t allocate_new_stack_size, const char *dbg_elf_path)
{
	RawPrefix *rp = (RawPrefix *) bi->start;

	char *ptr = rp->jump_region;

	uint32_t rel_offset;
	uint32_t rel_count;
	lookup_rel_section(es, &rel_offset, &rel_count, assert_rel_exists);

#define SPLAT(v)	{ memcpy(ptr, &(v), sizeof(v)); ptr+=sizeof(v); }

	if (allocate_new_stack_size != 0)
	{
		SPLAT(x86_push_ebx);
		SPLAT(x86_push_immediate);
		SPLAT(allocate_new_stack_size);

		SPLAT(x86_mov_deref_ebx_to_eax);
		uint8_t offset = offsetof(ZoogDispatchTable_v1, zoog_allocate_memory);
		SPLAT(offset);
		SPLAT(x86_call_indirect_eax);

		SPLAT(x86_pop_ecx);		// remove stack_size parameter
		SPLAT(x86_pop_ebx);

		SPLAT(x86_add_immediate_eax);
		uint32_t stack_headroom = 0x10;
		uint32_t stack_top = allocate_new_stack_size - stack_headroom;
		SPLAT(stack_top);

		SPLAT(x86_mov_eax_to_esp);

		// terminate the frame chain, to keep elfmain from "discovering"
		// an invalid top-of-stack.
		SPLAT(x86_mov_esp_to_ebp);
		SPLAT(x86_push_immediate);
		uint32_t zero = 0;
		SPLAT(zero);
	}

	SPLAT(x86_push_immediate);
	SPLAT(rel_count);
	SPLAT(x86_push_immediate);
	SPLAT(rel_offset);

	// Push on elfbase
	SPLAT(x86_call);
	uint32_t zero = 0;
	SPLAT(zero);
	// now stack top contains current PC.
	uint32_t offset_from_call_to_elfbase = sizeof(RawPrefix)  - (ptr - bi->start);
	SPLAT(x86_pop_eax);
	SPLAT(x86_add_immediate_eax);
	SPLAT(offset_from_call_to_elfbase);
	SPLAT(x86_push_eax);

	SPLAT(x86_jmp_relative);
	// TODO assuming elf is mapped at vaddr 0
	uint32_t next_instruction = ptr-bi->start+sizeof(uint32_t);
	uint32_t target_instruction = es->ehdr->e_entry + sizeof(RawPrefix);
	uint32_t offset_to_entry = target_instruction - next_instruction;
		// assuming everything works out right, this relative jump
		// is exactly "e_entry" bytes forward, since elf vaddr = 0
	SPLAT(offset_to_entry);

	assert(((uint32_t)(ptr-bi->start)) <= sizeof(rp->jump_region));	// Need to increase sizeof(RawPrefix.jump_region). A better program would compute this value at runtime, rather than having a compile-time value and a run-time assert that it was big enough.

	if (dbg_elf_path[0]!='/')
	{
		getcwd(rp->dbg_elf_path, sizeof(rp->dbg_elf_path));
		strncat(rp->dbg_elf_path, "/", sizeof(rp->dbg_elf_path));
	}
	else
	{
		rp->dbg_elf_path[0] = '\0';
	}
	strncat(rp->dbg_elf_path, dbg_elf_path, sizeof(rp->dbg_elf_path));
}

/*
	(1) make a flat memory image -- fill out "missing" areas of file that
	are normally handled by an elf-comprehending loader.
	(2) if specified, make byte 0 the entry point
	(prepending before elf header).
*/
void flattenElfToBinary(const char *image, ElfState *es, BinaryImage *bi, bool includeElfHeader, bool insert_entry_jump)
{
	if (mapfile(image, es)!=0)
	{
		assert(0);
	}

	SlurpState *slurpState = &bi->slurpState;
	slurpState->min_address = 0xffffffff;
	slurpState->past_max_address = 0;
	slurpState->section_count = 0;
	slurpState->section_table = NULL;

	Elf32_Shdr pseudo_header;
	memset(&pseudo_header, 0, sizeof(pseudo_header));

	Elf32_Shdr pseudo_section_table;
	memset(&pseudo_section_table, 0, sizeof(pseudo_header));

	for (slurpState->phase = 0; slurpState->phase<=1; slurpState->phase++)
	{
		if (slurpState->phase==1)
		{
			bi->size = slurpState->past_max_address - slurpState->min_address;
			if (insert_entry_jump)
			{
				bi->size += sizeof(RawPrefix);
				bi->jump_shift = sizeof(RawPrefix);
			}
			else
			{
				bi->jump_shift = 0;
			}
			bi->start = malloc(bi->size);
			assert(bi->start!=NULL);
			memset(bi->start, 0, bi->size);
#if DBG_VERBOSE
			fprintf(stderr, "elfFlatten: base: 0x%08x size %08x\n",
				slurpState->min_address, bi->size);
#endif // DBG_VERBOSE

			if (includeElfHeader)
			{
				slurpState->section_table = (Elf32_Shdr *)
					(bi->start
						+bi->jump_shift
						+pseudo_section_table.sh_addr - slurpState->min_address);
			}
			slurpState->section_count = 0;	// start counting again for insert
		}

		int shcount;
		for (shcount = 0; shcount < es->ehdr->e_shnum; shcount += 1)
		{
			Elf32_Shdr *shdr = (Elf32_Shdr*)
				(es->elfbase + es->ehdr->e_shoff + shcount*es->ehdr->e_shentsize);
			slurp_section(es, bi, shdr, shcount);
		}

		if (includeElfHeader)
		{
			if (slurpState->phase==0)
			{
				// Oddly, there's no section header describing the Elf header.
				// We'll just roll one up and slurp its contents.
				//
				// TODO danger brittle:
				// We actually want to grab the Elf32_Ehdr and the program headers,
				// which we optimistically hope are all the bytes between offset 0
				// and the first section.
				//
				// (Things are shenanigan-y here because we're trying to serve
				// two masters: we want enough of the Elf file to be able to
				// read its structure in elfLoader2, but zeroing the .bss means
				// explicitly overwriting other parts of the Elf; we're just
				// relying on that to be symbols and other stuff we don't need.)
				pseudo_header.sh_flags = SHF_ALLOC;		// don't ignore it
				pseudo_header.sh_type = SHT_NULL;	// want real bits
				pseudo_header.sh_offset = 0;		// starts at beginning of file
					// NB magic: we only use includeElfHeader mode when processing
					// ld.so, where we know v_addr starts at zero. That saves
					// us tracking v_addr and offset separately to figure out
					// where first real section starts.
				pseudo_header.sh_addr = et_get_symbol_base(es->elfbase);
				pseudo_header.sh_size = slurpState->min_address - pseudo_header.sh_addr;

#if DBG_VERBOSE
				fprintf(stderr, "elfFlatten: pseudo_header is %08x bytes\n", pseudo_header.sh_size);
#endif // DBG_VERBOSE

				pseudo_section_table.sh_flags = SHF_ALLOC;	// don't ignore it
				pseudo_section_table.sh_type = SHT_NOBITS;	// don't copy over records being written in by slurps of other sections
				pseudo_section_table.sh_addr = slurpState->past_max_address;
				pseudo_section_table.sh_offset = es->ehdr->e_shoff;
				pseudo_section_table.sh_size = es->ehdr->e_shentsize * es->ehdr->e_shnum;
#if DBG_VERBOSE
				fprintf(stderr,
					"elfFlatten: pseudo_section_table @%08x, for %08x bytes\n",
					pseudo_section_table.sh_addr, pseudo_section_table.sh_size);
#endif // DBG_VERBOSE
			}
			slurp_section(es, bi, &pseudo_header, MAX_SECTIONS-1);
			slurp_section(es, bi, &pseudo_section_table, MAX_SECTIONS-1);
		}
	}

	Elf32_Ehdr *out_ehdr = (Elf32_Ehdr *)
		(bi->start+bi->jump_shift+pseudo_header.sh_offset);
	out_ehdr->e_shoff = pseudo_section_table.sh_addr - slurpState->min_address;
	out_ehdr->e_shnum = slurpState->section_count;
#if DBG_VERBOSE
	fprintf(stderr, "cropped shnum (%08x) to %d\n",
		((uint32_t) &out_ehdr->e_shoff) - (uint32_t)(bi->start+bi->jump_shift),
		out_ehdr->e_shnum);
#endif // DBG_VERBOSE
	int new_shstrndx = slurpState->section_remap_table[out_ehdr->e_shstrndx];
#if DBG_VERBOSE
	fprintf(stderr, "remapped e_shstrndx from %d to %d\n",
		out_ehdr->e_shstrndx, new_shstrndx);
#endif // DBG_VERBOSE
	out_ehdr->e_shstrndx = new_shstrndx;
}

void rewriteManifestName(ElfState *es, BinaryImage *bi, const char *manifest_string)
{
	// Now we rewrite the eld.manifest_name field.
	//Elf32_Addr image_base = find_image_base(es);
	// above wrong because it goes to beginning of mmaping,
	// ignores Elf header truncation.
	Elf32_Addr image_base = bi->slurpState.min_address;

	fprintf(stderr, "elfFlatten: image_base: 0x%08x\n", image_base);
	
	int symsz_eld;
	uint32_t eld_offset = find_symbol(es, "eld", &symsz_eld) - image_base;
	assert(symsz_eld == sizeof(ELDynamic));
	ELDynamic *eld = (ELDynamic*) (bi->start+bi->jump_shift+eld_offset);
	if (!(strlen(manifest_string)+1 <= sizeof(eld->manifest_name)))
	{
		fprintf(stderr,
			"\n\n"
			"ERROR: manifest_string (%d) has outgrown its space (%d)\n"
			"Update ELDynamic.h.\n",
			strlen(manifest_string)+1, sizeof(eld->manifest_name));
		assert(0);
	}
	strcpy(eld->manifest_name, manifest_string);
	fprintf(stderr, "elfFlatten: manifest_name at file offset 0x%08x\n", eld->manifest_name - bi->start);
}

void writeBinary(BinaryImage *bi, const char *outname)
{
	// and now we can emit the file.
	unlink(outname);
	FILE *fp = fopen(outname, "w+");
	int rc = fwrite(bi->start, bi->size, 1, fp);
	assert(rc==1);
	fclose(fp);
}

void writeCSource(BinaryImage *bi, const char *dirname, const char *outname)
{
	const char *varname = outname;

	char out_c_name[1024];	// I <3 Buffer Overfl0ws!
	sprintf(out_c_name, "%s/%s.c", dirname, outname);
	char out_h_name[1024];
	sprintf(out_h_name, "%s/%s.h", dirname, outname);

	unlink(out_c_name);
	FILE *out_c_fp = fopen(out_c_name, "w");
	fprintf(out_c_fp, "char %s[%d] = {\n", varname, bi->size);
	int count;
	char *p;
	for (count=0, p=bi->start; count<bi->size; count++, p++)
	{
		fprintf(out_c_fp, "%d,", bi->start[count]);
		if (((count+1)%20)==0)
		{
			fprintf(out_c_fp, "\n");
		}
	}
	fprintf(out_c_fp, "};\n");
	fclose(out_c_fp);

	unlink(out_h_name);
	FILE *out_h_fp = fopen(out_h_name, "w");
	fprintf(out_h_fp, "extern char %s[%d];\n", varname, bi->size);
	fclose(out_h_fp);
}

void usage(char *progname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "%s [--mode=elfloader <manifest_string>|--mode=ldso <dirname>] <in> <out>\n", progname);
	exit(-1);
}

#define arg_assert(x)	((!(x)) ? usage(progname) : 1)
#define PEEK_ARG	( arg_assert(argc>0), *(argv) )
#define CONSUME_ARG	( arg_assert(argc>0), argc--, *(argv++) )

char *load_all(const char *path)
{
	int rc;
	struct stat buf;
	rc = stat(path, &buf);
	assert(rc==0);
	char *space = malloc(buf.st_size+1);
	FILE *fp = fopen(path, "r");
	rc = fread(space, buf.st_size, 1, fp);
	assert(rc==1);
	fclose(fp);
	space[buf.st_size] = '\0';
	return space;
}

int main(int argc, char **argv)
{
	Args args;
	args_init(&args, argc, argv);

	ElfState es;
	BinaryImage bi;

	flattenElfToBinary(
		args.input_image,
		&es,
		&bi,
		args.include_elf_header,
		args.insert_entry_jump);

	if (args.insert_entry_jump)
	{
		install_entry_jump(&es, &bi, args.assert_rel_exists, args.allocate_new_stack_size, args.input_image);
	}

	if (args.insert_manifest != NULL)
	{
		char *manifest_string = load_all(args.insert_manifest);
		rewriteManifestName(&es, &bi, manifest_string);
	}

	if (args.emit_binary != NULL)
	{
		writeBinary(&bi, args.emit_binary);
	}

	if (args.emit_c_dir != NULL)
	{
		assert(args.emit_c_symbol != NULL);
		writeCSource(&bi, args.emit_c_dir, args.emit_c_symbol);

		char debugName[700];
		snprintf(debugName, sizeof(debugName), "%s/%s.debugbin", args.emit_c_dir, args.emit_c_symbol);
		writeBinary(&bi, debugName);
	}
	return 0;
}
