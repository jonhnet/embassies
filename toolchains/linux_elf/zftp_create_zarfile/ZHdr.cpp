#include "ZHdr.h"

ZHdr::ZHdr(uint32_t zftp_lg_block_size)
{
	zhdr.z_magic = Z_MAGIC;
	zhdr.z_version = Z_VERSION;
	zhdr.z_path_num = -1;
	zhdr.z_path_off = -1;
	zhdr.z_path_entsz = -1;
	zhdr.z_strtab_off = -1;
	zhdr.z_strtab_len = -1;
	zhdr.z_dbg_zarfile_len = -1;
	zhdr.z_lg_block_size = zftp_lg_block_size;

	_phdr0 = NULL;
	_index_count = -1;
	_string_table = NULL;
	_chunk_table = NULL;
}

void ZHdr::set_dbg_zarfile_len(uint32_t dbg_zarfile_len)
{
	zhdr.z_dbg_zarfile_len = dbg_zarfile_len;
}

uint32_t ZHdr::get_size()
{
	return sizeof(zhdr);
}

const char* ZHdr::get_type()
{
	return "ZHdr";
}

void ZHdr::emit(FILE *fp)
{
	zhdr.z_path_num = _index_count;
	if (_phdr0 == NULL)
	{
		lite_assert(_index_count==0);
		zhdr.z_path_off = 0;	// Any offset legal; no data there anyway.
	}
	else
	{
		zhdr.z_path_off = _phdr0->get_location();
	}
	zhdr.z_path_entsz = sizeof(ZF_Phdr);
	zhdr.z_strtab_off = _string_table->get_location();
	zhdr.z_strtab_len = _string_table->get_size();
	zhdr.z_chunktab_off = _chunk_table->get_location();
	zhdr.z_chunktab_count = _chunk_table->get_count();
	zhdr.z_chunktab_entsz = sizeof(ZF_Chdr);

	lite_assert(zhdr.z_strtab_off != (uint32_t) -1);
	lite_assert(zhdr.z_strtab_len != (uint32_t) -1);
	lite_assert(zhdr.z_dbg_zarfile_len != (uint32_t) -1);

	int rc = fwrite(&zhdr, sizeof(zhdr), 1, fp);
	lite_assert(rc==1);
}
