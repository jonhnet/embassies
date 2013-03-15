//#include "Debug.h"	// jonh commented-out because it keeps toolchains/linux_elf/unit_tests from building core_file_test. Discuss.

#if USE_FILE_IO
#include <stdio.h>
#endif // USE_FILE_IO
//#include <string.h>
#include "corefile.h"
#include "LiteLib.h"

void corefile_init(CoreFile *c, MallocFactory *mf)
{
	c->mf = mf;

	lite_memset(&c->corenote_zoog, 0, sizeof(c->corenote_zoog));
	c->corenote_zoog.note.nhdr.n_namesz = 5;
	c->corenote_zoog.note.nhdr.n_descsz = sizeof(c->corenote_zoog)-sizeof(NoteHeader);
	c->corenote_zoog.note.nhdr.n_type = 0x20092009;	// nonsense value
	c->corenote_zoog.note.name[0] = 'Z';
	c->corenote_zoog.note.name[1] = 'O';
	c->corenote_zoog.note.name[2] = 'O';
	c->corenote_zoog.note.name[3] = 'G';
	c->corenote_zoog.note.name[4] = '\0';
	c->corenote_zoog.bootblock_addr = 0x0;

	linked_list_init(&c->threads, mf);
	linked_list_init(&c->segments, mf);
}

void corefile_add_thread(CoreFile *c, Core_x86_registers *regs, int thread_id)
{
	CoreNote_Regs *note_regs = mf_malloc(c->mf, sizeof(CoreNote_Regs)); 

	lite_memset(note_regs, 0, sizeof(*note_regs));
	note_regs->note.nhdr.n_namesz = 5;
	note_regs->note.nhdr.n_descsz = sizeof(CoreNote_Regs) - sizeof(NoteHeader);
	note_regs->note.nhdr.n_type = NT_PRSTATUS;
	//strcpy(note_regs.name, "CORE"); 
	// Avoid VS warning that strcpy is unsafe
	note_regs->note.name[0] = 'C';
	note_regs->note.name[1] = 'O';
	note_regs->note.name[2] = 'R';
	note_regs->note.name[3] = 'E';
	note_regs->note.name[4] = '\0';
	lite_memcpy(&note_regs->desc.regs, regs, sizeof(Core_x86_registers));

	note_regs->desc.pid = thread_id;
	note_regs->desc.ppid = 0x101;
	note_regs->desc.pgrp = 0x102;
	note_regs->desc.sid = 0x103;

	linked_list_insert_tail(&c->threads, note_regs);
}

void corefile_add_segment(CoreFile *c, uint32_t vaddr, void *bytes, int size)
{
	CoreSegment *seg = (CoreSegment *) mf_malloc(c->mf, sizeof(CoreSegment));
	seg->vaddr = vaddr;
	seg->bytes = bytes;
	seg->size = size;
	linked_list_insert_tail(&c->segments, seg);
}

void corefile_set_bootblock_info(CoreFile *c, void *bootblock_addr, const char *bootblock_path)
{
	c->corenote_zoog.bootblock_addr = (uint32_t) bootblock_addr;
	lite_assert(lite_strlen(bootblock_path)+1 <= sizeof(c->corenote_zoog.bootblock_path));
	lite_strcpy(c->corenote_zoog.bootblock_path, bootblock_path);
}

#if USE_FILE_IO
bool _corefile_fwrite_callback(void *user_file_obj, void *ptr, int len)
{
	FILE *fp = (FILE*) user_file_obj;
	int rc = fwrite(ptr, len, 1, fp);
	return rc==1;
}

int _corefile_ftell_callback(void *user_file_obj)
{
	FILE *fp = (FILE*) user_file_obj;
	return ftell(fp);
}

void corefile_write(FILE *fp, CoreFile *c)
{
	corefile_write_custom(c, _corefile_fwrite_callback, _corefile_ftell_callback, fp);
}
#endif // USE_FILE_IO

bool _corefile_size_counter_cb(void *user_file_obj, void *ptr, int len)
{
	uint32_t *total = (uint32_t*) user_file_obj;
	(*total) += len;
	return true;
}

uint32_t corefile_compute_size(CoreFile *c)
{
	uint32_t total = 0;
	corefile_write_custom(c, _corefile_size_counter_cb, NULL, &total);
	return total;
}

void corefile_write_custom(CoreFile *c, write_callback_func *write_func, tell_callback_func *ftell_func, void *user_file_obj)
{
//	write_func(user_file_obj, "slicksnot", 9);
	const int num_notes = 1;
	int pad_i;
	int hdr_offset = 0;
	bool rc;
	Elf32_Ehdr ehdr;	

	int thread_notes_size = c->threads.count * sizeof(CoreNote_Regs);

	// compute how much header space we need...
	int hdr_size = 0
		+sizeof(Elf32_Ehdr)
		+sizeof(Elf32_Phdr)					// NOTE hdr
		+sizeof(Elf32_Phdr)*c->segments.count
		+thread_notes_size
		+sizeof(c->corenote_zoog);
	int rounded_hdr_size = (hdr_size + 0xfff) & ~0xfff;
	int pad_size = rounded_hdr_size - hdr_size;

	// ...so that we can plan on laying data segments down after that.
	int seg_file_offset = rounded_hdr_size;

	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	ehdr.e_ident[EI_OSABI] = ELFOSABI_NONE;
	ehdr.e_ident[EI_ABIVERSION] = 0;
	
	for (pad_i=EI_PAD; pad_i<EI_NIDENT; pad_i++)
	{
		ehdr.e_ident[pad_i] = 0;
	}
	ehdr.e_type = ET_CORE;
	ehdr.e_machine = EM_386;
	ehdr.e_version = EV_CURRENT;
	ehdr.e_entry = 0;
	ehdr.e_phoff = sizeof(ehdr);
	ehdr.e_shoff = 0;
	ehdr.e_flags = 0;
	ehdr.e_ehsize = sizeof(ehdr);
	ehdr.e_phentsize = sizeof(Elf32_Phdr);
	ehdr.e_phnum = (Elf32_Half) (num_notes + c->segments.count);
	ehdr.e_shentsize = 0;
	ehdr.e_shnum = 0;
	ehdr.e_shstrndx = 0;

	rc = write_func(user_file_obj, &ehdr, sizeof(ehdr));
	lite_assert(rc);
	hdr_offset += sizeof(ehdr);

	// write CoreNotes Phdr
	{
		lite_assert(c->corenote_zoog.bootblock_addr != 0x0);

		Elf32_Phdr phdr;
		phdr.p_type = PT_NOTE;
		phdr.p_flags = 0;
		phdr.p_offset = hdr_offset + sizeof(Elf32_Phdr)*ehdr.e_phnum;
		phdr.p_vaddr = 0;
		phdr.p_paddr = 0;
		phdr.p_filesz = thread_notes_size + sizeof(c->corenote_zoog);
		phdr.p_memsz = 0;
		phdr.p_align = 0;

		rc = (*write_func)(user_file_obj, &phdr, sizeof(phdr));
		lite_assert(rc);
		hdr_offset += sizeof(phdr);
	}

	LinkedListIterator lli;
	for (ll_start(&c->segments, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		CoreSegment *seg = (CoreSegment *) ll_read(&lli);

		Elf32_Phdr phdr;
		lite_assert((seg->size & 0xfff) == 0);
		
		phdr.p_type = PT_LOAD;
		phdr.p_flags = PF_X|PF_W|PF_R;
		phdr.p_offset = seg_file_offset;
		phdr.p_vaddr = (uint32_t) seg->vaddr;
		phdr.p_paddr = 0;
		phdr.p_filesz = seg->size;
		phdr.p_memsz = seg->size;
		phdr.p_align = 0x1000;

		rc = (*write_func)(user_file_obj, &phdr, sizeof(phdr));
		lite_assert(rc);
		hdr_offset += sizeof(phdr);

		seg_file_offset += phdr.p_filesz;
		lite_assert(ftell_func==NULL || hdr_offset == (*ftell_func)(user_file_obj));
	}

	// write the PT_NOTEs that contains the register & zoog symbol info
	{
		LinkedListIterator lli;
		for (ll_start(&c->threads, &lli);
			ll_has_more(&lli);
			ll_advance(&lli))
		{
			CoreNote_Regs *corenote_regs = (CoreNote_Regs *) ll_read(&lli);

			rc = (*write_func)(user_file_obj, corenote_regs, sizeof(*corenote_regs));
			lite_assert(rc);
			hdr_offset += sizeof(*corenote_regs);
		}
		rc = (*write_func)(user_file_obj, &c->corenote_zoog, sizeof(c->corenote_zoog));
		lite_assert(rc);
		hdr_offset += sizeof(c->corenote_zoog);
	}

	lite_assert(pad_size>=0);

	lite_assert(pad_size < 0x1000);
	char pad_zeros[0x1000];
	lite_memset(pad_zeros, 0, sizeof(pad_zeros));
	rc = (*write_func)(user_file_obj, &pad_zeros, pad_size);
	lite_assert(rc);
	lite_assert(ftell_func==NULL || rounded_hdr_size == (*ftell_func)(user_file_obj));
	seg_file_offset = rounded_hdr_size;

	for (ll_start(&c->segments, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		CoreSegment *seg = (CoreSegment *) ll_read(&lli);

		rc = (*write_func)(user_file_obj, seg->bytes, seg->size);
		lite_assert(rc);
		seg_file_offset += seg->size;
		lite_assert(ftell_func==NULL || seg_file_offset == (*ftell_func)(user_file_obj));
	}
}

