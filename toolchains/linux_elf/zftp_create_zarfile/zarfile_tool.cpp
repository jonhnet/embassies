#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "zarfile.h"
#include "zftp_dir_format.h"
#include "ZTArgs.h"
#include "PosixChunkDecoder.h"
#include "FileScanner.h"

class ListChunksOperation : public FileOperationIfc {
private:
	uint32_t bytes_in_stat;
	uint32_t bytes_in_file;
	uint32_t bytes_in_dir;
public:
	ListChunksOperation();
	void operate(FileScanner *scanner);
	int result();
};

ListChunksOperation::ListChunksOperation()
	: bytes_in_stat(0),
	  bytes_in_file(0),
	  bytes_in_dir(0)
{
}

void ListChunksOperation::operate(FileScanner *scanner)
{
	bool is_dir =
		scanner->get_phdr()->protocol_metadata.flags & ZFTP_METADATA_FLAG_ISDIR;

	uint32_t total_chunk_bytes = 0;
	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);
		total_chunk_bytes += chunk.z_data_len;
	}

	char buffer[1000];
	snprintf(buffer, sizeof(buffer), "%c %10d %10d %s",
		((scanner->get_phdr()->protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
			? 'E'
				: (is_dir ? 'd' : 'f')),
		scanner->get_phdr()->z_file_len,
		total_chunk_bytes,
		scanner->get_path());
//	fprintf(stdout, "%c %10d %10d %s\n",
//		((scanner->get_phdr()->protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
//			? 'E'
//				: (is_dir ? 'd' : 'f')),
//		scanner->get_phdr()->z_file_len,
//		total_chunk_bytes,
//		scanner->get_path());

	uint32_t *bill_to = NULL;
	if (lite_starts_with(STAT_SCHEME, scanner->get_path()))
	{
		bill_to = &bytes_in_stat;
	}
	else if (is_dir)
	{
		bill_to = &bytes_in_dir;
	}
	else
	{
		bill_to = &bytes_in_file;
	}

	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);
		//fprintf(stdout, "  chunk @0x%x :0x%x%s (file offset %x)\n",
		fprintf(stdout, "chunk @0x%08x :0x%08x file_offset 0x%08x %-9s %s \n", 
			chunk.z_data_off,
			chunk.z_data_len,
			chunk.z_file_off,
			chunk.z_precious ? "precious" : "fast",
			 buffer);
		(*bill_to) += chunk.z_data_len;
	}
}

int ListChunksOperation::result()
{
	fprintf(stdout, "Summary: stat %d file %d dir %d bytes\n",
		bytes_in_stat, bytes_in_file, bytes_in_dir);
	return 0;
}

class FlatUnpack : public FileOperationIfc {
private:
	const char* dest_path;

public:
	FlatUnpack(const char* dest_path)
		: dest_path(dest_path)
		{}
	int result();
	void operate(FileScanner *scanner);
};

int FlatUnpack::result()
{
	return 0;
}

void FlatUnpack::operate(FileScanner *scanner)
{
	bool is_dir = (scanner->get_phdr()->protocol_metadata.flags
		& ZFTP_METADATA_FLAG_ISDIR)!=0;
	if (is_dir)
	{
		return;
	}
	if (strncmp(scanner->get_path(), "file:", 5) != 0)
	{
		return;
	}

	char* in = strdup(&scanner->get_path()[5]);
	char* base = basename(in);
	int out_len = strlen(dest_path) + 1 + strlen(base) + 20 + 1;
	char* out_path = (char*) malloc(out_len);

	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);

		snprintf(out_path, out_len, "%s/%s-c%04d.chunk", dest_path, base, ci);
		FILE* fp = fopen(out_path, "w");

		uint8_t* buf = (uint8_t*)malloc(chunk.z_data_len);
		scanner->read_zarfile(buf, chunk.z_data_len, chunk.z_file_off);
		int rc = fwrite(buf, chunk.z_data_len, 1, fp);
		assert(rc==1);
		free(buf);

		fclose(fp);
	}

	free(out_path);
	free(in);
}

class ExtractOperation : public FileOperationIfc {
private:
	const char *_path;
	bool _completed;
	bool decode_as_directory;

	void decode_directory(ChunkDecoder* pcd, uint32_t len);

public:
	ExtractOperation(const char *path, bool decode_as_directory)
		: _path(path),
		 _completed(false),
		 decode_as_directory(decode_as_directory)
		{}
	int result();
	void operate(FileScanner *scanner);
};

void ExtractOperation::decode_directory(ChunkDecoder* pcd, uint32_t len)
{
	ZFTPDirectoryRecord drec;
	uint32_t offset = 0;

	while (offset < len)
	{
		pcd->read(&drec, sizeof(drec), offset);

		int name_len = drec.reclen - sizeof(drec);
		char *name = (char*) malloc(name_len);
		pcd->read(name, name_len, offset+sizeof(drec));
		fprintf(stdout, "%s\n", name);
		free(name);
		offset += drec.reclen;
	}
	lite_assert(offset==len);
}

void ExtractOperation::operate(FileScanner *scanner)
{
	if (strcmp(scanner->get_path(), _path)==0)
	{
		ChunkDecoder* chunk_decoder = scanner->get_chunk_decoder();
		if (decode_as_directory)
		{
			decode_directory(chunk_decoder, scanner->get_phdr()->z_file_len);
		}
		else
		{
			lite_assert(scanner->get_phdr()->z_file_len < 100<<20);	// >100MB? Probably should write an incremental copy.
			void* buf = malloc(scanner->get_phdr()->z_file_len);
			chunk_decoder->read(buf, scanner->get_phdr()->z_file_len, 0);
			int rc;
			rc = fwrite(buf, scanner->get_phdr()->z_file_len, 1, stdout);
			lite_assert(rc==1);
			free(buf);
		}

		scanner->interrupt();
		_completed = true;

		delete chunk_decoder;
	}
}

int ExtractOperation::result()
{
	if (!_completed)
	{
		fprintf(stderr, "Couldn't find path %s\n", _path);
		return 255;
	}
	return 0;
}


class ListFilesOperation : public FileOperationIfc {
private:
	uint32_t bytes_in_stat;
	uint32_t bytes_in_file;
	uint32_t bytes_in_dir;
	bool verbose;
public:
	ListFilesOperation(bool verbose);
	void operate(FileScanner *scanner);
	int result();
};

ListFilesOperation::ListFilesOperation(bool verbose)
	: bytes_in_stat(0),
	  bytes_in_file(0),
	  bytes_in_dir(0),
	  verbose(verbose)
{
}

void ListFilesOperation::operate(FileScanner *scanner)
{
	bool is_dir =
		scanner->get_phdr()->protocol_metadata.flags & ZFTP_METADATA_FLAG_ISDIR;

	uint32_t total_chunk_bytes = 0;
	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);
		total_chunk_bytes += chunk.z_data_len;
	}

	fprintf(stdout, "%c %10d %10d %s\n",
		((scanner->get_phdr()->protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
			? 'E'
				: (is_dir ? 'd' : 'f')),
		scanner->get_phdr()->z_file_len,
		total_chunk_bytes,
		scanner->get_path());
	if (verbose)
	{
//fprintf(stdout, "	chunk_idx %0x 
	}

	uint32_t *bill_to = NULL;
	if (lite_starts_with(STAT_SCHEME, scanner->get_path()))
	{
		bill_to = &bytes_in_stat;
	}
	else if (is_dir)
	{
		bill_to = &bytes_in_dir;
	}
	else
	{
		bill_to = &bytes_in_file;
	}

	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);
		fprintf(stdout, "  chunk @0x%x :0x%x%s (file offset %x)\n",
			chunk.z_data_off,
			chunk.z_data_len,
			chunk.z_precious ? " (precious)" : " (fast)",
			chunk.z_file_off);
		(*bill_to) += chunk.z_data_len;
	}
}

int ListFilesOperation::result()
{
	fprintf(stdout, "Summary: stat %d file %d dir %d bytes\n",
		bytes_in_stat, bytes_in_file, bytes_in_dir);
	return 0;
}

int main(int argc, char **argv)
{
	ZTArgs args(argc, argv);
	
	FileOperationIfc *op;
	switch (args.mode)
	{
	case ZTArgs::list_files:
		op = new ListFilesOperation(args.verbose);
		break;
	case ZTArgs::list_chunks:
		op = new ListChunksOperation();
		break;
	case ZTArgs::extract:
		op = new ExtractOperation(args.extract_path, false);
		break;
	case ZTArgs::list_dir:
		op = new ExtractOperation(args.dir_path, true);
		break;
	case ZTArgs::flat_unpack:
		op = new FlatUnpack(args.flat_unpack_dest);
		break;
	default:
		lite_assert(false);
	}
	FileScanner scanner(args.zarfile, op);
	int result = op->result();
	delete op;

	return result;
}
