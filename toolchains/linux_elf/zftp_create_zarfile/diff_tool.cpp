#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "zarfile.h"
#include "copy_file.h"
#include "DTArgs.h"
#include "Set.h"
#include "standard_malloc_factory.h"
#include "HashableHash.h"

class IOException { };

class Scanner {
private:
	const char *path;
public:
	Scanner(const char *path);
	void scan();

	Set unique_blocks;
	int block_count;
};

Scanner::Scanner(const char *path)
	: path(path),
	  unique_blocks(standard_malloc_factory_init()),
	  block_count(0)
{
}

void Scanner::scan()
{
	FILE *fp = fopen(path, "r");
	if (fp==NULL)
	{
		throw IOException();
	}

	uint8_t *buf = (uint8_t*) malloc(ZFTP_BLOCK_SIZE);

	int blk;
	for (blk=0; ; blk++)
	{
		int rc;
		rc = fread(buf, 1, ZFTP_BLOCK_SIZE, fp);
		if (rc<0)
		{
			throw IOException();
		}
		else if (rc==0)
		{
			break;
		}
		else if (rc>ZFTP_BLOCK_SIZE)
		{
			assert(false);
		}
		else if (rc<ZFTP_BLOCK_SIZE)
		{
			memset(buf+rc, 0, ZFTP_BLOCK_SIZE-rc);
		}

		block_count += 1;
		hash_t block_hash;
		zhash(buf, ZFTP_BLOCK_SIZE, &block_hash);
		HashableHash *hash = new HashableHash(&block_hash, blk*ZFTP_BLOCK_SIZE);
		HashableHash* existing = (HashableHash*) unique_blocks.lookup(hash);
		if (existing==NULL)
		{
			unique_blocks.insert(hash);
		}
		else
		{
			existing->increment();
			delete hash;
		}
	}

	free(buf);
}

int blocks_to_mb(int blocks)
{
	return (blocks<<ZFTP_LG_BLOCK_SIZE)>>20;
}

int blocks_to_kb(int blocks)
{
	return (blocks<<ZFTP_LG_BLOCK_SIZE)>>10;
}

class BlockPrinter
{
public:
	int fi;
public:
	BlockPrinter(int fi) : fi(fi) {}
	static void s_visit(void* v_this, void* v_obj)
	{
		((BlockPrinter*) v_this)->visit((HashableHash*) v_obj);
	}
	void visit(HashableHash* h)
	{
		fprintf(stderr, "The block at 0x%08x (%d copies) is unique to file %d\n",
			h->get_found_at_offset(),
			h->get_num_occurrences(),
			fi);
	}
};

int main(int argc, char **argv)
{
	DTArgs args(argc, argv);

	Scanner *s[2];
	for (int i=0; i<2; i++)
	{
		s[i] = new Scanner(args.file[i]);
		s[i]->scan();
	}

	Set *common = s[0]->unique_blocks.intersection(&s[1]->unique_blocks);

	for (int i=0; i<2; i++)
	{
		int j = 1-i;
		fprintf(stderr,
			"file %d: %s\n",
			i,
			basename(args.file[i]));
		fprintf(stderr, "  %d blocks (%d MB)\n",
			s[i]->block_count,
			blocks_to_mb(s[i]->block_count));
		fprintf(stderr, "  %d blocks (%d MB) unique\n",
			s[i]->unique_blocks.count(),
			blocks_to_mb(s[i]->unique_blocks.count()));
		fprintf(stderr, "  %d common with file %d (%d MB)\n",
			common->count(),
			j,
			blocks_to_mb(common->count()));

		// NB this is an underestimate; it should sum the get_num_occurrences()
		// of each in the difference set.
		int blocks_diff = s[i]->unique_blocks.count() - common->count();
		fprintf(stderr, "  %d different with file %d (%d kB, %.1f%%)\n",
			blocks_diff,
			j,
			blocks_to_kb(blocks_diff),
			common->count()*100.0 / s[i]->unique_blocks.count());

		Set *diff = s[i]->unique_blocks.difference(&s[j]->unique_blocks);
		BlockPrinter printer(i);
		diff->visit(BlockPrinter::s_visit, &printer);
	}
	
	return 0;
}
