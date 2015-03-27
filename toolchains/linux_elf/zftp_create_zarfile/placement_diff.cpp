#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "zarfile.h"
#include "PDArgs.h"
#include "FileScanner.h"
#include "ChunkRecord.h"
#include "SyncFactory_Pthreads.h"

class ScanChunksOperation : public FileOperationIfc {
private:
	ChunkRecordTree tree;

public:
	ScanChunksOperation(SyncFactory *sf);
	void operate(FileScanner *scanner);
	int result();
	ChunkRecordTree* get_tree() { return &tree; }
};

ScanChunksOperation::ScanChunksOperation(SyncFactory *sf)
	: tree(sf)
{
}

void ScanChunksOperation::operate(FileScanner *scanner)
{
	for (uint32_t ci=0; ci<scanner->get_phdr()->z_chunk_count; ci++)
	{
		ZF_Chdr chunk;
		scanner->read_chunk(scanner->get_phdr()->z_chunk_idx + ci, &chunk);
		uint32_t start = chunk.z_data_off;
		uint32_t end = start + chunk.z_data_len;
		ChunkRecord* record = new ChunkRecord(
			scanner->get_path(), true, DataRange(start, end), chunk.z_file_off);
		tree.insert(record);
	}
}

int ScanChunksOperation::result()
{
	return 0;
}

class PlacementDiff
{
private:
	SyncFactory_Pthreads sf;
	ScanChunksOperation* sco[2];
	int mass[2];
	int partial_mass[2];

	void scan(PDArgs* args);
	void compute_total_mass(int i);
	void diff();
	ChunkRecord* consume_matching_placement(ChunkRecord* proto);

public:
	PlacementDiff(PDArgs* args);
};

PlacementDiff::PlacementDiff(PDArgs* args)
{
	scan(args);
	diff();
}

void PlacementDiff::scan(PDArgs* args)
{
	for (int i=0; i<2; i++)
	{
		sco[i] = new ScanChunksOperation(&sf);
		FileScanner scanner(args->file[i], sco[i]);
		sco[i]->result();

		compute_total_mass(i);
	}
}

void PlacementDiff::compute_total_mass(int i)
{
	mass[i] = 0;
	ChunkRecord* ptr = sco[i]->get_tree()->findMin();
	while (ptr!=NULL)
	{
		mass[i] += ptr->get_data_range().size();
		ptr = sco[i]->get_tree()->findFirstGreaterThan(ptr);
	}
}
	
void PlacementDiff::diff()
{
	partial_mass[0] = 0;
	partial_mass[1] = 0;

	while (true)
	{
		// "min" here means "biggest chunks first",
		// because ChunkRecords sort descending.
		ChunkRecord* min[2];
		min[0] = sco[0]->get_tree()->findMin();
		min[1] = sco[1]->get_tree()->findMin();
		if (min[0]==NULL && min[1]==NULL)
		{
			// chunks all observed
			break;
		}

		// ref indexes the chunk we'll be studying next.
		// It's possible (in fact, we hope frequent!) for min[0]==min[1],
		// in which case we select arbitrarily.
		int ref;
		if (min[0]==NULL)
		{
			ref = 1;
		}
		else if (min[1]==NULL)
		{
			ref = 0;
		}
		else if (*min[0] < *min[1])
		{
			ref = 0;
		}
		else
		{
			ref = 1;
		}

		int other = 1-ref;
		const char* disposition = NULL;
		ChunkRecord* affected_chunk = NULL;
		if (min[other]!=NULL && min[other]->content_equals(min[ref]))
		{
			// See if the same content appears in any two matching locations.
			// They may not be in any particular 'instance' order, so we
			// need to iterate through any duplicates in both lists.
			ChunkRecord* found_match = consume_matching_placement(min[ref]);
			if (found_match!=NULL)
			{
				disposition = "undisturbed";
				affected_chunk = found_match;
			}
			else
			{
				// same content appears in both files, but not in any
				// matching location.
				// remove an arbitrary instance from each side;
				// because we're in the 'else' case, we must have
				// already removed any existing matches, so we're not
				// perturbing the results in future loop iterations.
				sco[ref]->get_tree()->remove(min[ref]);
				affected_chunk = min[ref];
				sco[other]->get_tree()->remove(min[other]);
				delete(min[other]);
				disposition = "disturbed";
			}
			partial_mass[0] += affected_chunk->get_data_range().size();
			partial_mass[1] += affected_chunk->get_data_range().size();
		}
		else
		{
			// this chunk only appears in one file.
			sco[ref]->get_tree()->remove(min[ref]);
			affected_chunk = min[ref];
			disposition = ref==0 ? "only in 0" : "only in 1";
			partial_mass[ref] += affected_chunk->get_data_range().size();
		}

		int frac[2];
		// NB need the floating-point math to avoid overflowing int32_t
		frac[0] = partial_mass[0]*100.0/mass[0];
		frac[1] = partial_mass[1]*100.0/mass[1];

		fprintf(stdout,
			"%-12s: %08x @%08x 0:%2d%% 1:%2d%%\n    %s\n",
			disposition,
			affected_chunk->get_data_range().size(),
			affected_chunk->get_data_range().start(),
			frac[0],
			frac[1],
			affected_chunk->get_url());
		delete affected_chunk;
	}
}

// n^2 scan -- assumes n typically small, which is a valid assumption;
// we don't duplicate chunks too many times.
ChunkRecord* PlacementDiff::consume_matching_placement(ChunkRecord* proto)
{
	ChunkRecord* ptr[2];
	for (ptr[0]=sco[0]->get_tree()->findMin();
		ptr[0]!=NULL && ptr[0]->content_equals(proto);
		ptr[0]=sco[0]->get_tree()->findFirstGreaterThan(ptr[0]))
	{
		for (ptr[1]=sco[1]->get_tree()->findMin();
			ptr[1]!=NULL && ptr[1]->content_equals(proto);
			ptr[1]=sco[1]->get_tree()->findFirstGreaterThan(ptr[1]))
		{
			if (ptr[0]->get_location()==ptr[1]->get_location())
			{
				// found a match! remove both; delete one, return t'other.
				sco[1]->get_tree()->remove(ptr[1]);
				delete ptr[1];
				sco[0]->get_tree()->remove(ptr[0]);
				return ptr[0];
			}
		}
	}
	return NULL;
}

int main(int argc, char **argv)
{
	PDArgs args(argc, argv);
	PlacementDiff pd(&args);
	return 0;
}
