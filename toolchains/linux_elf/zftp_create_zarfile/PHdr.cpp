#include <assert.h>
#include "PHdr.h"

PHdr::PHdr(uint32_t len, uint32_t flags)
{
	memset(&phdr, -1, sizeof(phdr));
 
	phdr.z_path_str_off = -1;
	phdr.z_path_str_len = -1;
	phdr.z_file_len = len;
	phdr.z_chunk_idx = -1;
	phdr.z_chunk_count = -1;
	phdr.protocol_metadata.file_len_network_order = z_htonl(len);
	phdr.protocol_metadata.flags = flags;
}

uint32_t PHdr::get_size()
{
	return sizeof(phdr);
}

void PHdr::connect_string(uint32_t string_index, uint32_t string_len)
{
	phdr.z_path_str_off = string_index;
	phdr.z_path_str_len = string_len;
}

void PHdr::connect_chunks(uint32_t chunk_index, uint32_t chunk_count)
{
	phdr.z_chunk_idx = chunk_index;
	phdr.z_chunk_count = chunk_count;
}

void PHdr::emit(FILE *fp)
{
	int rc = fwrite(&phdr, sizeof(phdr), 1, fp);
	assert(rc==1);
}

uint32_t PHdr::get_file_len()
{
	return phdr.z_file_len;
}
