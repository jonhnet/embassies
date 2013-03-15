#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "zarfile.h"
#include "zftp_dir_format.h"
#include "ZTArgs.h"
#include "PosixChunkDecoder.h"

class Scanner;

class Operation {
public:
	virtual void operate(Scanner *scanner) = 0;
	virtual int result() = 0;
};

class Scanner {
public:
	Scanner(const char *zarfile, Operation *operation);
	void read_chunk(uint32_t chunk_idx, ZF_Chdr *out_chunk);

	FILE *ifp;
	ZF_Zhdr zhdr;
	ZF_Phdr phdr;
	char path[800];
	bool interrupt;
};

Scanner::Scanner(const char *zarfile, Operation *operation)
{
	interrupt = false;

	ifp = fopen(zarfile, "r");
	assert(ifp!=NULL);
	int rc;

	rc = fread(&zhdr, sizeof(zhdr), 1, ifp);
	assert(rc==1);

	assert(sizeof(ZF_Phdr)==zhdr.z_path_entsz);
	assert(zhdr.z_magic == Z_MAGIC);
	assert(zhdr.z_version == Z_VERSION);

	uint32_t pi;
	for (pi=0; pi<zhdr.z_path_num && !interrupt; pi++)
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
}

void Scanner::read_chunk(uint32_t chunk_idx, ZF_Chdr *out_chunk)
{
	assert(zhdr.z_chunktab_entsz == sizeof(*out_chunk));
	assert(chunk_idx < zhdr.z_chunktab_count);
	int rc;
	rc = fseek(ifp, zhdr.z_chunktab_off + chunk_idx*zhdr.z_chunktab_entsz, SEEK_SET);
	assert(rc==0);

	rc = fread(out_chunk, zhdr.z_chunktab_entsz, 1, ifp);
	assert(rc==1);
}

class ListChunksOperation : public Operation {
private:
	uint32_t bytes_in_stat;
	uint32_t bytes_in_file;
	uint32_t bytes_in_dir;
public:
	ListChunksOperation();
	void operate(Scanner *scanner);
	int result();
};

ListChunksOperation::ListChunksOperation()
	: bytes_in_stat(0),
	  bytes_in_file(0),
	  bytes_in_dir(0)
{
}

void ListChunksOperation::operate(Scanner *scanner)
{
	bool is_dir =
		scanner->phdr.protocol_metadata.flags & ZFTP_METADATA_FLAG_ISDIR;

	uint32_t total_chunk_bytes = 0;
	for (uint32_t ci=0; ci<scanner->phdr.z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->phdr.z_chunk_idx + ci, &chunk);
		total_chunk_bytes += chunk.z_data_len;
	}

	char buffer[1000];
	snprintf(buffer, sizeof(buffer), "%c %10d %10d %s",
		((scanner->phdr.protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
			? 'E'
				: (is_dir ? 'd' : 'f')),
		scanner->phdr.z_file_len,
		total_chunk_bytes,
		scanner->path);
//	fprintf(stdout, "%c %10d %10d %s\n",
//		((scanner->phdr.protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
//			? 'E'
//				: (is_dir ? 'd' : 'f')),
//		scanner->phdr.z_file_len,
//		total_chunk_bytes,
//		scanner->path);

	uint32_t *bill_to = NULL;
	if (lite_starts_with(STAT_SCHEME, scanner->path))
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

	for (uint32_t ci=0; ci<scanner->phdr.z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->phdr.z_chunk_idx + ci, &chunk);
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

class ExtractOperation : public Operation {
private:
	const char *_path;
	bool _completed;
	bool decode_as_directory;

	void decode_directory(PosixChunkDecoder* pcd, uint32_t len);

public:
	ExtractOperation(const char *path, bool decode_as_directory)
		: _path(path),
		 _completed(false),
		 decode_as_directory(decode_as_directory)
		{}
	int result();
	void operate(Scanner *scanner);
};

void ExtractOperation::decode_directory(PosixChunkDecoder* pcd, uint32_t len)
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

void ExtractOperation::operate(Scanner *scanner)
{
	if (strcmp(scanner->path, _path)==0)
	{
		PosixChunkDecoder pcd(&scanner->zhdr, &scanner->phdr, scanner->ifp);
		if (decode_as_directory)
		{
			decode_directory(&pcd, scanner->phdr.z_file_len);
		}
		else
		{
			lite_assert(scanner->phdr.z_file_len < 100<<20);	// >100MB? Probably should write an incremental copy.
			void* buf = malloc(scanner->phdr.z_file_len);
			pcd.read(buf, scanner->phdr.z_file_len, 0);
			int rc;
			rc = fwrite(buf, scanner->phdr.z_file_len, 1, stdout);
			lite_assert(rc==1);
			free(buf);
		}

		scanner->interrupt = true;
		_completed = true;
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


class ListFilesOperation : public Operation {
private:
	uint32_t bytes_in_stat;
	uint32_t bytes_in_file;
	uint32_t bytes_in_dir;
public:
	ListFilesOperation();
	void operate(Scanner *scanner);
	int result();
};

ListFilesOperation::ListFilesOperation()
	: bytes_in_stat(0),
	  bytes_in_file(0),
	  bytes_in_dir(0)
{
}

void ListFilesOperation::operate(Scanner *scanner)
{
	bool is_dir =
		scanner->phdr.protocol_metadata.flags & ZFTP_METADATA_FLAG_ISDIR;

	uint32_t total_chunk_bytes = 0;
	for (uint32_t ci=0; ci<scanner->phdr.z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->phdr.z_chunk_idx + ci, &chunk);
		total_chunk_bytes += chunk.z_data_len;
	}

	fprintf(stdout, "%c %10d %10d %s\n",
		((scanner->phdr.protocol_metadata.flags&ZFTP_METADATA_FLAG_ENOENT)
			? 'E'
				: (is_dir ? 'd' : 'f')),
		scanner->phdr.z_file_len,
		total_chunk_bytes,
		scanner->path);

	uint32_t *bill_to = NULL;
	if (lite_starts_with(STAT_SCHEME, scanner->path))
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

	for (uint32_t ci=0; ci<scanner->phdr.z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->phdr.z_chunk_idx + ci, &chunk);
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
	
	Operation *op;
	switch (args.mode)
	{
	case ZTArgs::list_files:
		op = new ListFilesOperation();
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
	default:
		lite_assert(false);
	}
	Scanner scanner(args.zarfile, op);
	int result = op->result();
	delete op;

	return result;
}
