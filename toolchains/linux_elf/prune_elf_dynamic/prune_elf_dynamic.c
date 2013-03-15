#include <string.h>
#include <alloca.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>

#include "elfobj.h"
#include "elf_flat_reader.h"

int main(int argc, char **argv)
{
	assert(argc==3);
	char *in_fn = argv[1];
	char *out_fn = argv[2];

	ElfFlatReader efr;
	efr_read_file(&efr, in_fn);

	ElfObj elfobj, *eo=&elfobj;
	bool rc = elfobj_scan(eo, &efr.ifc);
	assert(rc);	// scan failed

//	eo->ehdr->e_type = ET_EXEC;
	ElfWS_Shdr *dyn_sh = elfobj_find_shdr_by_name(eo, ".dynamic", NULL);
	ElfWS_Dyn *dyn_src = (ElfWS_Dyn *) elfobj_read_section_by_name(eo, ".dynamic", NULL);
	ElfWS_Dyn *dyn_dst = (ElfWS_Dyn *) alloca(dyn_sh->sh_size);
	memset(dyn_dst, 0, dyn_sh->sh_size);
	int src_count = dyn_sh->sh_size/sizeof(ElfWS_Dyn);

	int src_i, dst_i;
	for (src_i=0, dst_i=0; src_i < src_count; src_i++)
	{
		bool gobble = false;
		const char *type = NULL;
		if (dyn_src[src_i].d_tag == DT_INIT)
		{
			gobble = true;
			type = "DT_INIT";
		}
		// wheezy apparently has a fancier compiler/linker with
		// a new way of expressing the DT_INIT bug...
		if (dyn_src[src_i].d_tag == DT_INIT_ARRAY)
		{
			gobble = true;
			type = "DT_INIT_ARRAY";
		}
		if (dyn_src[src_i].d_tag == DT_FINI)
		{
			gobble = true;
			type = "DT_FINI";
		}
		if (dyn_src[src_i].d_tag == DT_RPATH)
		{
			gobble = true;
			type = "DT_RPATH";
		}

		if (gobble)
		{
			printf("Gobbling %s at %d\n", type, src_i);
		}
		else
		{
			dyn_dst[dst_i] = dyn_src[src_i];
			dst_i++;
		}
	}

	// now copy into place, leaving holes at the end of the section
	memcpy(dyn_src, dyn_dst, dyn_sh->sh_size);
	// and tell the section header we've cropped the .dynamic section.
	dyn_sh->sh_size = dst_i * sizeof(ElfWS_Dyn);

	efr_write_file(&efr, out_fn);
	chmod(out_fn, 0755);

	return 0;
}
