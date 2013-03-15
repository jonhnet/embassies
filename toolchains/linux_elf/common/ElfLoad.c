#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdbool.h>

#include "LiteLib.h"
#include "ElfLoad.h"

Elf32_Addr round_down(Elf32_Addr addr, Elf32_Word p_align)
{
	return addr & ~(p_align-1);
}

Elf32_Addr round_up(Elf32_Addr addr, Elf32_Word p_align)
{
	Elf32_Addr out = addr & ~(p_align-1);
	if (out!=addr)
	{
		out += p_align;
	}
	return out;
}

void clean_up(Elf32_Addr start, Elf32_Word size, char *name)
{
	//printf("zeroing %s %08x %08x\n", name, start, size);
	char *p, *e;
	for (p=(char*)start, e=(char*) (start+size); p<e; p++)
	{
		*p = 0;
	}

	lite_memset((void*) start, 0, size);
}

ElfLoaded scan_elf(void *scan_addr)
{
	ElfLoaded el = {-1, NULL, -1};
	el.ehdr = (Elf32_Ehdr*) scan_addr;

	int phcount;
	for (phcount = 0; phcount < el.ehdr->e_phnum; phcount += 1)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*)
			(scan_addr + el.ehdr->e_phoff + phcount*el.ehdr->e_phentsize);
		if (phdr->p_type == PT_LOAD)
		{
			// TODO copy-pasted code
			Elf32_Addr mapping_start = round_down(phdr->p_vaddr, phdr->p_align);
			Elf32_Addr data_start = phdr->p_vaddr;
//			Elf32_Addr data_end = phdr->p_vaddr+phdr->p_filesz;
			Elf32_Addr mapping_end = round_up(phdr->p_vaddr+phdr->p_memsz, phdr->p_align);
			Elf32_Addr image_end = round_up(phdr->p_vaddr+phdr->p_filesz, phdr->p_align);
			lite_assert(image_end<=mapping_end);

			long data_offset = phdr->p_offset-(data_start-mapping_start);

			if (el.base==-1)
			{
				if (data_offset == mapping_start)
				{
					el.displace = (Elf32_Addr) scan_addr;
				}
				else
				{
					el.displace = 0;
				}
				el.base = mapping_start+el.displace;
			}
		}
	}
	lite_assert(el.base!=-1);
	lite_assert(el.displace!=-1);
	return el;
}

#if USE_FILE_IO

void map_one_segment(ElfLoaded *el, Elf32_Phdr *phdr, bool copyin, Elf32_Addr base, int fd)
{
	if (phdr->p_type != PT_LOAD)
	{
		return;
	}

	Elf32_Addr mapping_start = round_down(phdr->p_vaddr, phdr->p_align);
	Elf32_Addr data_start = phdr->p_vaddr;
	Elf32_Addr data_end = phdr->p_vaddr+phdr->p_filesz;
	Elf32_Addr mapping_end = round_up(phdr->p_vaddr+phdr->p_memsz, phdr->p_align);
	Elf32_Addr image_end = round_up(phdr->p_vaddr+phdr->p_filesz, phdr->p_align);
	lite_assert(image_end<=mapping_end);

	long data_offset = phdr->p_offset-(data_start-mapping_start);

	if ((el->displace == 0) && (data_offset == mapping_start))
	{
		// want to map file address 0 to memory address 0? Then
		// we get to displace contents somewhere else in memory.
		el->displace = (Elf32_Addr) base;
	}

	if (!copyin)
	{
		Elf32_Addr segment_begin = mapping_start+el->displace;
		Elf32_Addr segment_end = mapping_end + el->displace;
		if (el->all_begin == -1 || segment_begin < el->all_begin)
		{
			el->all_begin = segment_begin;
		}
		if (el->all_end == -1 || segment_end > el->all_end)
		{
			el->all_end = segment_end;
		}
	}
	else
	{
		//printf("virtbase %08x size %08x\n", mapping_start+el.displace, mapping_end-mapping_start);
		void *mrc;
		mrc = mmap(
			(void*) mapping_start+el->displace,
			image_end-mapping_start,
			PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE|MAP_FIXED,
			// TODO jonh too lazy to set flags correctly
			fd,
			data_offset);
		lite_assert(mrc==(void*) mapping_start+el->displace);

		clean_up(data_end+el->displace, image_end-data_end, "suffix");

		if (image_end<mapping_end)
		{
			printf("allocating anonymous tail pages for %s at %08x\n",
				el->dbg_fn, image_end);
			mrc = mmap(
				(void*) image_end+el->displace,
				mapping_end - image_end,
				PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,
				-1,
				0);
			lite_assert(mrc==(void*)image_end+el->displace);
		}
	}

	if (el->base==-1)
	{
		// unfounded optimism:
		// first mapping probably starts at beginning! wee!
		el->base = mapping_start+el->displace;
		el->ehdr = (Elf32_Ehdr *) el->base;
		lite_assert(data_offset == 0);
	}
}

ElfLoaded load_elf(const char *fn)
{
	int rc;
	int fd = open(fn, O_RDONLY);
	lite_assert(fd>=0);

#define VIEW_HEADERS_VIA_MMAP	0
	// don't mmap the whole thing; that's expensive inside XPEland
#if VIEW_HEADERS_VIA_MMAP
	struct stat buf;
	rc = fstat(fd, &buf);
	lite_assert(rc==0);
	void *scan_addr = mmap(0, buf.st_size, PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE, fd, 0);

	ElfLoaded el = {-1, NULL, 0, -1, -1, fn};
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) scan_addr;

	void* phdrs = scan_addr + ehdr->e_phoff;
#else
	ElfLoaded el = {-1, NULL, 0, -1, -1, fn};
	Elf32_Ehdr ehdr_alloc, *ehdr = &ehdr_alloc;
	rc = read(fd, ehdr, sizeof(*ehdr));
	lite_assert(rc==sizeof(*ehdr));	// short/failed read of elf header

	int phdrs_size = ehdr->e_phentsize * ehdr->e_phnum;
	// malloc poisonous for zguest (a client of this code),
	// and alloca is generally freaky.
	char phdr_space[8192];
	lite_assert(phdrs_size <= sizeof(phdr_space));
	void* phdrs = phdr_space;
	rc = lseek(fd, ehdr->e_phoff, SEEK_SET);
	lite_assert(rc==ehdr->e_phoff);
	rc = read(fd, phdrs, phdrs_size);
	lite_assert(rc==phdrs_size);
#endif

	// scan
	int phcount;
	for (phcount = 0; phcount < ehdr->e_phnum; phcount += 1)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*) (phdrs+phcount*ehdr->e_phentsize);
		map_one_segment(&el, phdr, false, 0, /* fd not needed this pass */ -1);
	}

	// allocate
	void *mrc;
	mrc = mmap(NULL, el.all_end - el.all_begin,
		PROT_READ|PROT_WRITE|PROT_EXEC,
		MAP_PRIVATE|MAP_ANONYMOUS,
		/* fd */ -1,
		/* offset */ 0);
	el.base = (Elf32_Addr) mrc;
	el.ehdr = mrc;

	// map
	for (phcount = 0; phcount < ehdr->e_phnum; phcount += 1)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*) (phdrs+phcount*ehdr->e_phentsize);
		map_one_segment(&el, phdr, true, (Elf32_Addr) mrc, fd);
	}
	
#if VIEW_HEADERS_VIA_MMAP
	munmap(scan_addr, buf.st_size);
#else
#endif

	el.ehdr = (Elf32_Ehdr*) el.base;

	return el;
}
#endif // USE_FILE_IO

