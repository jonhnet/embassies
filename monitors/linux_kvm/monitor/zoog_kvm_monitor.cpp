#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>
#include <assert.h>
#include <malloc.h>
#include <fcntl.h>

#include "ZoogVM.h"
#include "elf_flat_reader.h"
#include "elfobj.h"
#include "xax_network_utils.h"

#include "standard_malloc_factory.h"
#include "malloc_factory_operator_new.h"
#include "MonitorArgs.h"
#include "MmapOverride.h"
#include "ambient_malloc.h"

class ElfMapper
{
public:
	ElfMapper(ElfObj *eo, uint8_t *buf);
	uint32_t get_size();
	uint32_t get_entry_offset();
	uint32_t get_pseudo_load_address(uint32_t guest_base);

private:
	void _map_one(Elf32_Phdr *phdr);
	void _scan();

	ElfObj *eo;
	uint8_t *buf;
	bool virt_base_valid;
	uint32_t virt_base;
	uint32_t virt_max;
};

ElfMapper::ElfMapper(ElfObj *eo, uint8_t *buf)
{
	this->eo = eo;
	this->buf = buf;
	this->virt_base_valid = false;
	_scan();
}

void ElfMapper::_map_one(Elf32_Phdr *phdr)
{
	if (phdr->p_type!=PT_LOAD)
	{
		return;
	}
	if (!virt_base_valid)
	{
		virt_base = phdr->p_vaddr;
		virt_max = phdr->p_vaddr;
		virt_base_valid = true;
	}
	uint32_t mem_start_offset = phdr->p_vaddr - virt_base;
	uint32_t virt_end = phdr->p_vaddr + phdr->p_memsz;
	if (virt_end > virt_max)
	{
		virt_max = virt_end;
	}
	if (buf!=NULL)
	{
		uint32_t file_offset = phdr->p_offset;
		(eo->eri->elf_read)(eo->eri,
			buf+mem_start_offset, file_offset, phdr->p_filesz);
		if (phdr->p_memsz > phdr->p_filesz)
		{
			uint32_t zero_offset = mem_start_offset+phdr->p_filesz;
			uint32_t zero_size = phdr->p_memsz - phdr->p_filesz;
			memset(buf+zero_offset, 0, zero_size);
		}
	}
}

void ElfMapper::_scan()
{
	for (int pi=0; pi<eo->ehdr->e_phnum; pi++)
	{
		Elf32_Phdr *phdr = &eo->phdr[pi];
		_map_one(phdr);
	}
}

uint32_t ElfMapper::get_entry_offset()
{
	return eo->ehdr->e_entry - virt_base;
}

uint32_t ElfMapper::get_pseudo_load_address(uint32_t guest_base)
{
	// There's a lot of weird chicanery going on here. We're loading the
	// file in flat, not laying it out according to the virtaddrs in
	// the elf file. So we have to lie to gdb as if we'd loaded the file
	// earlier in memory, I guess. Ugh, what a mess.
	return guest_base - virt_base;
}

uint32_t ElfMapper::get_size()
{
	return virt_max - virt_base;
}

void load_elf_pal(ZoogVM *vm, char *path)
{
	ElfFlatReader efr;
	efr_read_file(&efr, path);

	ElfObj eo;
	elfobj_scan(&eo, &efr.ifc);

	ElfMapper em_count(&eo, NULL);
	uint8_t *mem_image = (uint8_t*) malloc(em_count.get_size());
	ElfMapper em_load(&eo, mem_image);

	uint32_t mapped_base = vm->map_image(mem_image, em_count.get_size(), "pal_image");
	uint32_t entry_point_guest = mapped_base + em_count.get_entry_offset();
	vm->set_guest_entry_point(entry_point_guest);

	FILE *fp = fopen("debug_mmap", "w");
	fprintf(fp, "P %s %08x\n", path, em_load.get_pseudo_load_address(mapped_base));
	fclose(fp);
	
	free(mem_image);
}

void load_app(ZoogVM *vm, const char *path)
{
	// NB using ElfFlatReader for its reading abilities;
	// there's nothing ELFy about the app binary. It's a zoog SignedBinary.
	ElfFlatReader efr;
	efr_read_file(&efr, path);
	uint8_t *contents = (uint8_t*) (efr.ifc.elf_read_alloc)
		(&efr.ifc, 0, efr.ifc.size);
	SignedBinary *sb = (SignedBinary*) contents;
	assert(Z_NTOHG(sb->magic) == 0x5a53474e);
	vm->map_app_code(
		contents+sizeof(SignedBinary)+Z_NTOHG(sb->cert_len),
		Z_NTOHG(sb->binary_len),
		path);
	ZCert *cert = new ZCert(
		contents+sizeof(SignedBinary), Z_NTOHG(sb->cert_len));
	vm->set_pub_key(cert->getEndorsingKey());
}

int main(int argc, const char **argv)
{
	// get MmapOverride installed before we need memory!
	MmapOverride mmapOverride;

	fprintf(stderr, "Parsing args.\n");
	MonitorArgs args(argc, (char**) argv);
	fprintf(stderr, "Done parsing args.\n");

	if (args.wait_for_debugger)
	{
		fprintf(stderr, "monitor starts; waiting for debugger.\n");
	}
	while (args.wait_for_debugger)
	{
		sleep(1);
	}

	MallocFactory *mf = standard_malloc_factory_init();
	malloc_factory_operator_new_init(mf);
	ambient_malloc_init(mf);

	ZoogVM *zvm = new ZoogVM(mf, &mmapOverride, args.wait_for_core);
	load_elf_pal(zvm,
		(char*) ZOOG_ROOT "/monitors/linux_kvm/pal/build/zoog_kvm_pal");

	load_app(zvm, args.image_file);
	if (args.delete_image_file)
	{
		int rc = unlink(args.image_file);
		assert(rc==0);
	}

	zvm->start();
}

uint32_t getCurrentTime() {
  return (uint32_t) time (NULL);
}
