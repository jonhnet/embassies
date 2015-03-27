#include <assert.h>
#include "FileScanner.h"
#include "PosixChunkDecoder.h"

FileScanner::FileScanner(const char *zarfile, FileOperationIfc *operation)
{
	_interrupted = false;

	ifp = fopen(zarfile, "r");
	assert(ifp!=NULL);
	int rc;

	rc = fread(&zhdr, sizeof(zhdr), 1, ifp);
	assert(rc==1);

	assert(sizeof(ZF_Phdr)==zhdr.z_path_entsz);
	assert(zhdr.z_magic == Z_MAGIC);
	assert(zhdr.z_version == Z_VERSION);

	operation->operate_zhdr(&zhdr);

	uint32_t pi;
	for (pi=0; pi<zhdr.z_path_num && !_interrupted; pi++)
	{
		rc = fseek(ifp, zhdr.z_path_off+zhdr.z_path_entsz*pi, SEEK_SET);
		assert(rc==0);

		rc = fread(&phdr, sizeof(phdr), 1, ifp);
		assert(rc==1);

		rc = fseek(ifp, zhdr.z_strtab_off+phdr.z_path_str_off, SEEK_SET);
		assert(rc==0);

		assert(phdr.z_path_str_len+1 <= sizeof(path));
		rc = fread(path, phdr.z_path_str_len+1, 1, ifp);
		assert(rc==1);

		operation->operate(this);
	}
	fclose(ifp);
}

void FileScanner::read_zarfile(uint8_t* buf, uint32_t len, uint32_t offset)
{
	int rc;
	rc = fseek(ifp, offset, SEEK_SET);
	assert(rc==0);

	rc = fread(buf, len, 1, ifp);
	assert(rc==1);
}

void FileScanner::read_chunk(uint32_t chunk_idx, ZF_Chdr *out_chunk)
{
	assert(zhdr.z_chunktab_entsz == sizeof(*out_chunk));
	assert(chunk_idx < zhdr.z_chunktab_count);
	read_zarfile(
		(uint8_t*) out_chunk,
		zhdr.z_chunktab_entsz,
		zhdr.z_chunktab_off + chunk_idx*zhdr.z_chunktab_entsz);
}

ChunkDecoder* FileScanner::get_chunk_decoder()
{
	return new PosixChunkDecoder(&zhdr, &phdr, ifp);
}
