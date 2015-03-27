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
#include "zlc_util.h"
#include "linked_list.h"
#include "Blaminator.h"

class IOException { };

class BlockScanner {
private:
	const char *path;
	bool print_hashes;
	uint32_t zftp_block_size;

public:
	BlockScanner(const char *path, bool print_hashes, uint32_t zftp_block_size);
	void scan();

	Set unique_blocks;
	int block_count;
	LinkedList hash_list;
};

BlockScanner::BlockScanner(const char *path, bool print_hashes, uint32_t zftp_block_size)
	: path(path),
	  print_hashes(print_hashes),
	  zftp_block_size(zftp_block_size),
	  unique_blocks(standard_malloc_factory_init()),
	  block_count(0)
{
	linked_list_init(&hash_list, standard_malloc_factory_init());
}

void BlockScanner::scan()
{
	FILE *fp = fopen(path, "r");
	if (fp==NULL)
	{
		throw IOException();
	}

	uint8_t *buf = (uint8_t*) malloc(zftp_block_size);

	int blk;
	for (blk=0; ; blk++)
	{
		int rc;
		rc = fread(buf, 1, zftp_block_size, fp);
		if (rc<0)
		{
			throw IOException();
		}
		else if (rc==0)
		{
			break;
		}
		else if (rc>(int)zftp_block_size)
		{
			assert(false);
		}
		else if (rc<(int)zftp_block_size)
		{
			memset(buf+rc, 0, zftp_block_size-rc);
		}

		block_count += 1;
		hash_t block_hash;
		zhash(buf, zftp_block_size, &block_hash);

		if (print_hashes)
		{
			char hashbuf[sizeof(hash_t)*2+1];
			debug_hash_to_str(hashbuf, &block_hash);
			fprintf(stderr, "%s[%d] == %s\n", path, blk, hashbuf);
		}

		HashableHash *new_hash = new HashableHash(&block_hash, blk*zftp_block_size);
		HashableHash* existing = (HashableHash*) unique_blocks.lookup(new_hash);
		HashableHash* selected_ref = NULL;
		if (existing==NULL)
		{
			unique_blocks.insert(new_hash);
			selected_ref = new_hash;
		}
		else
		{
			existing->increment();
			selected_ref = existing;
			delete new_hash;
		}
		linked_list_insert_tail(&hash_list, selected_ref);
	}

	free(buf);
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

class DiffTool
{
private:
	DTArgs* args;
	uint32_t zftp_lg_block_size;
	uint32_t zftp_block_size;
	BlockScanner *s[2];
	void position_independent_comparison();
	void position_dependent_comparison();

	void blame(int file_idx, DataRange range, HashableHash* hash, const char* msg, int other_file_idx);

	int blocks_to_mb(int blocks);
	int blocks_to_kb(int blocks);
	uint32_t read_zftp_lg_block_size(const char* path);

public:
	DiffTool(DTArgs* args);
};

int DiffTool::blocks_to_mb(int blocks)
{
	return (blocks<<zftp_lg_block_size)>>20;
}

int DiffTool::blocks_to_kb(int blocks)
{
	return (blocks<<zftp_lg_block_size)>>10;
}

uint32_t DiffTool::read_zftp_lg_block_size(const char* path)
{
	FILE *fp = fopen(path, "r");
	if (fp==NULL)
	{
		return -1;
	}
	ZF_Zhdr zhdr;
	int rc;
	rc = fread(&zhdr, sizeof(zhdr), 1, fp);
	if (rc!=1)
	{
		return -1;
	}
	fclose(fp);
	return zhdr.z_lg_block_size;
}

DiffTool::DiffTool(DTArgs* args)
	: args(args),
	  zftp_block_size(0)
{
	uint32_t zftp_lg_block_sizes[2];

	for (int i=0; i<2; i++)
	{
		zftp_lg_block_sizes[i] = read_zftp_lg_block_size(args->file[i]);
		s[i] = new BlockScanner(args->file[i], args->print_hashes, 1<<(zftp_lg_block_sizes[i]));
		s[i]->scan();
	}

	if (zftp_lg_block_sizes[0]!=zftp_lg_block_sizes[1])
	{
		fprintf(stderr, "Trying to compare zarfiles built with non-matching zftp_lg_block_size (%d, %d); results would be meaningless.\n",
			zftp_lg_block_sizes[0],
			zftp_lg_block_sizes[1]);
		exit(-1);
	}

	assert(zftp_lg_block_sizes[0]==zftp_lg_block_sizes[1]);
	zftp_lg_block_size = zftp_lg_block_sizes[0];
	zftp_block_size = 1<<zftp_lg_block_size;

	if (args->position_dependent)
	{
		position_dependent_comparison();
	}
	else
	{
		position_independent_comparison();
	}
}

void DiffTool::position_independent_comparison()
{
	Set *common = s[0]->unique_blocks.intersection(&s[1]->unique_blocks);

	for (int i=0; i<2; i++)
	{
		int j = 1-i;
		fprintf(stderr,
			"file %d: %s\n",
			i,
			basename(args->file[i]));
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
			blocks_diff*100.0 / s[i]->unique_blocks.count());

		if (args->list_blocks)
		{
			Set *diff = s[i]->unique_blocks.difference(&s[j]->unique_blocks);
			BlockPrinter printer(i);
			diff->visit(BlockPrinter::s_visit, &printer);
		}
	}
}

void DiffTool::blame(int file_idx, DataRange range, HashableHash* hash, const char* msg, int other_file_idx)
{
	fprintf(stdout, "hash from %d @%x..%x %s %d\n", file_idx, range.start(), range.end(), msg, other_file_idx);
	{
		char hashbuf[sizeof(hash_t)*2+1];
		debug_hash_to_str(hashbuf, hash->get_hash());
		fprintf(stdout, "hash value is %s\n", hashbuf);
	}
	fprintf(stdout, "dd if=%s bs=%d skip=%d count=1 > /tmp/block%d\n",
		args->file[file_idx],
		zftp_block_size,
		range.start()/zftp_block_size,
		file_idx);

	Blaminator blaminator(range);
	FileScanner scanner(args->file[file_idx], &blaminator);
	fprintf(stdout, "\n");
}

void DiffTool::position_dependent_comparison()
{
	uint32_t compared_count = 0;
	uint32_t match_count = 0;
	LinkedListIterator lli[2];

	for (
		ll_start(&s[0]->hash_list, &lli[0]),
			ll_start(&s[1]->hash_list, &lli[1]);
		ll_has_more(&lli[0]) && ll_has_more(&lli[1]);
		ll_advance(&lli[0]), ll_advance(&lli[1]))
	{
		HashableHash* h[2];
		h[0] = (HashableHash*) ll_read(&lli[0]);
		h[1] = (HashableHash*) ll_read(&lli[1]);
		bool match = h[0]->cmp(h[1])==0;
		if (match)
		{
			match_count += 1;
		}
		else if (args->blame_mismatches)
		{
			int i;
			for (i=0; i<=1; i++)
			{
				DataRange range(compared_count*zftp_block_size, (compared_count+1)*zftp_block_size);
				int j=1-i;
				if (s[j]->unique_blocks.contains(h[i]))
				{
					blame(i, range, h[i], "appears elsewhere in file", j);
				}
				else
				{
					blame(i, range, h[i], "absent from file", j);
				}
			}
		}
		compared_count += 1;
	}
	fprintf(stderr, "%d match, %d differ, %d\n", match_count, compared_count-match_count, compared_count);
}

int main(int argc, char **argv)
{
	DTArgs args(argc, argv);

	DiffTool tool(&args);
	return 0;
}
