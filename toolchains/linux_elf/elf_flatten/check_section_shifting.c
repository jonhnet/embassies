#include <assert.h>
#include <elf.h>

#include "elfobj.h"
#include "elf_flat_reader.h"

int main(int argc, char **argv)
{
	assert(argc==2);
	ElfFlatReader efr;
	efr_read_file(&efr, argv[1]);

	ElfObj elfobj, *eo=&elfobj;
	elfobj_scan(eo, &efr.ifc);

	int i;
	for (i=0; i<eo->ehdr->e_shnum; i++)
	{
		printf("[%d] offset 0x%6x size 0x%6x type %d\n",
			i,
			eo->shdr[i].sh_offset,
			eo->shdr[i].sh_size,
			eo->shdr[i].sh_type);
	}

	return 0;
}
