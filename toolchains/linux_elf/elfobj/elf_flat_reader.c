#include <string.h>

#if ELFOBJ_USE_LIBC
#include <malloc.h>
#include <sys/stat.h>
#endif // ELFOBJ_USE_LIBC

#include "LiteLib.h"
#include "elf_flat_reader.h"

bool _efr_read(ElfReaderIfc *eri, uint8_t *buf, uint32_t offset, uint32_t length)
{
	ElfFlatReader *efr = (ElfFlatReader *) eri;
	if (offset+length > efr->ifc.size)
	{
		return false;
	}
	memcpy(buf, &efr->bytes[offset], length);
	return true;
}

void *_efr_read_alloc(ElfReaderIfc *eri, uint32_t offset, uint32_t length)
{
	ElfFlatReader *efr = (ElfFlatReader *) eri;
	if (offset+length > efr->ifc.size)
	{
		return NULL;
	}
	return &efr->bytes[offset];
}

void _efr_dtor(ElfReaderIfc *eri)
{
	ElfFlatReader *efr = (ElfFlatReader *) eri;
	if (efr->this_owns_image)
	{
#if ELFOBJ_USE_LIBC
		free(efr->bytes);
#else
		lite_assert(0);
#endif
	}
}

void elf_flat_reader_init(ElfFlatReader *efr, uint8_t *bytes, uint32_t size, bool this_owns_image)
{
	efr->ifc.elf_read = _efr_read;
	efr->ifc.elf_read_alloc = _efr_read_alloc;
	efr->ifc.elf_dtor = _efr_dtor;
	efr->ifc.size = size;
	efr->bytes = bytes;
	efr->this_owns_image = this_owns_image;
}

#if ELFOBJ_USE_LIBC
#include <assert.h>

void efr_read_file(ElfFlatReader *efr, const char *filename)
{
	int rc;
	uint32_t image_size;
	void *image;

	FILE *fp = fopen(filename, "r");
	if (fp==NULL) {
		image = malloc(1);
		image_size = 0;
	}
	else
	{
		struct stat statbuf;
		rc = fstat(fileno(fp), &statbuf);
		assert(rc==0);
		image_size = statbuf.st_size;
		image = malloc(image_size);
		if (image_size > 0)
		{
			rc = fread(image, image_size, 1, fp);
			assert(rc==1);
		}
		fclose(fp);
	}

	elf_flat_reader_init(efr, image, image_size, true);
}

void efr_write_file(ElfFlatReader *efr, const char *filename)
{
	int rc;
	FILE *ofp = fopen(filename, "w");
	rc = fwrite(efr->bytes, efr->ifc.size, 1, ofp);
	assert(rc==1);
	fclose(ofp);
}
#endif // ELFOBJ_USE_LIBC
