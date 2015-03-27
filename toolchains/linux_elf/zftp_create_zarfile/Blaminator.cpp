#include "Blaminator.h"

void Blaminator::operate_zhdr(ZF_Zhdr *zhdr)
{
	DataRange zhdr_range(0, sizeof(ZF_Zhdr));
	if (range.intersects(zhdr_range)) {
		fprintf(stdout, "  The ZHdr\n");
	}

	DataRange strtab_range(zhdr->z_strtab_off, zhdr->z_strtab_off+zhdr->z_strtab_len);
	if (range.intersects(strtab_range)) {
		fprintf(stdout, "  The strtab\n");
	}

	DataRange chunktab_range(zhdr->z_chunktab_off, zhdr->z_chunktab_off+zhdr->z_chunktab_count*zhdr->z_chunktab_entsz);
	if (range.intersects(strtab_range)) {
		fprintf(stdout, "  The chunktab\n");
	}
}

void Blaminator::operate(FileScanner *scanner)
{
	ZF_Phdr* phdr = scanner->get_phdr();
	for (uint32_t ci=0; ci<phdr->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(phdr->z_chunk_idx + ci, &chunk);
		DataRange chunk_range(chunk.z_file_off, chunk.z_file_off+chunk.z_data_len);
		if (range.intersects(chunk_range)) {
			fprintf(stdout, "  File %s, chunk %d sz %08x\n",
				scanner->get_path(), ci, chunk.z_data_len);
		}
	}
}

int Blaminator::result()
{
	return 0;
}

