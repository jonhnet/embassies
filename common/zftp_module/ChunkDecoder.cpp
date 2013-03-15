#include "LiteLib.h"
#include "ChunkDecoder.h"
#include "math_util.h"

void ChunkDecoder::read_chdr(int local_chunk_idx, ZF_Chdr *out_chdr)
{
	uint32_t chdr_offset =
		(phdr->z_chunk_idx + local_chunk_idx)*zhdr->z_chunktab_entsz
		+ zhdr->z_chunktab_off;
	underlying_zarfile_read(out_chdr, sizeof(ZF_Chdr), chdr_offset);
}

void ChunkDecoder::read(void *buf, uint32_t count, uint32_t offset)
{
	lite_assert(offset+count <= phdr->z_file_len);

	uint8_t* buf8 = (uint8_t*) buf;
	uint32_t buf_offset = 0;
	uint32_t file_offset = offset;
	uint32_t file_count = count;
	while (file_count > 0)
	{
		while (true)
		{
			bool progress = false;
			for (uint32_t ci = 0; ci<phdr->z_chunk_count; ci++)
			{
				ZF_Chdr chdr;
				read_chdr(ci, &chdr);
				uint32_t chunk_start = chdr.z_data_off;
				uint32_t chunk_end = chdr.z_data_off+chdr.z_data_len;
				if (file_offset < chunk_start || file_offset >= chunk_end)
				{
					continue;
				}

				uint32_t chunk_avail = chunk_end - file_offset;
				uint32_t this_read_len = min(file_count, chunk_avail);
				if (this_read_len == 0) { continue; }
				progress = true;
				underlying_zarfile_read(
					buf8+buf_offset,
					this_read_len,
					chdr.z_file_off + (file_offset - chdr.z_data_off));

				buf_offset += this_read_len;
				file_offset += this_read_len;
				file_count -= this_read_len;
				if (file_count==0) { break; }
			}
			if (!progress) { break; }
			// else we've advanced the read pointer, so rescanning chunk list
			// may still be profitable; loop around.
		}
		if (file_count==0) { break; }
		lite_assert(false);	// read not satisfied by any of the chunks of data
		// in the zarfile. Perhaps because we consumed one in a fast_mmap?
		// Need a graceful way to recover! (Be nice to drop down to an
		// underlying ZFTP request.)
	}
}

