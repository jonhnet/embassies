#include <stdio.h>
#include "zarfile.h"
#include "FileOperationIfc.h"
#include "ChunkDecoder.h"

class FileScanner {
private:
	FILE *ifp;
	ZF_Zhdr zhdr;
	ZF_Phdr phdr;
	char path[800];
	bool _interrupted;

public:
	FileScanner(const char *zarfile, FileOperationIfc *operation);

	// upcalls from operation->operate()
	void read_zarfile(uint8_t* buf, uint32_t len, uint32_t offset);
	void read_chunk(uint32_t chunk_idx, ZF_Chdr *out_chunk);
	ZF_Zhdr* get_zhdr() { return &zhdr; }
	// these change with each call to operate()
	ZF_Phdr* get_phdr() { return &phdr; }
	char* get_path() { return path; }
	ChunkDecoder* get_chunk_decoder();
		// better be careful to delete the result before you return from
		// operate()!
	void interrupt() { _interrupted = true; }
};
