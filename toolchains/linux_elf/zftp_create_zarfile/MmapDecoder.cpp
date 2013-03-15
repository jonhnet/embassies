#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "MmapDecoder.h"
#include "standard_malloc_factory.h"

//////////////////////////////////////////////////////////////////////////////

bool MDRange::equals(MDRange &other)
{
	return start==other.start && end==other.end;
}

bool MDRange::includes(MDRange &other)
{
	return other.start >= start && other.end <= end;
}

//////////////////////////////////////////////////////////////////////////////

MmapBehavior::MmapBehavior()
	: filename(NULL),
		aligned_mapping_size(0),
		aligned_mapping_copies(0)
{
	linked_list_init(&precious_ranges, standard_malloc_factory_init());
}

uint32_t MmapBehavior::hash(const void* datum)
{
	MmapBehavior* obj = (MmapBehavior*) datum;
	return hash_buf(obj->filename, strlen(obj->filename));
}

int MmapBehavior::cmp(const void *v_a, const void *v_b)
{
	MmapBehavior* a = (MmapBehavior*) v_a;
	MmapBehavior* b = (MmapBehavior*) v_b;
	return strcmp(a->filename, b->filename);
}

void MmapBehavior::insert_range(MDRange* a_range)
{
	bool matches_existing = false;

	LinkedListIterator lli;
	for (ll_start(&precious_ranges, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		MDRange* range = (MDRange*) ll_read(&lli);
		if (range->includes(*a_range))
		{
			matches_existing = true;
			break;
		}
	}
	// TODO not very precise; doesn't coalesce ranges or anything. But
	// seems adequate.
	if (!matches_existing)
	{
		MDRange *new_range = new MDRange(*a_range);
		linked_list_insert_tail(&precious_ranges, new_range);
	}
}

//////////////////////////////////////////////////////////////////////////////

MmapDecoder::MmapDecoder(const char* debug_mmap_filename)
	: broken(false)
{
	hash_table_init(&ht, standard_malloc_factory_init(), MmapBehavior::hash, MmapBehavior::cmp);

	memset(fd_table, 0, sizeof(fd_table));
	parse(debug_mmap_filename);
}

void MmapDecoder::parse(const char* debug_mmap_filename)
{
	FILE *fp = fopen(debug_mmap_filename, "r");
	if (fp==NULL)
	{
		fprintf(stderr, "WARNING: No mmap cue file '%s' present.\n",
			debug_mmap_filename);
		return;
	}

	char buf[800];
	uint32_t line_num = 0;
	while (true) {
		char *line = fgets(buf, sizeof(buf), fp);
		line_num += 1;
		if (line==NULL)
		{
			break;
		}
		else if (line[0]=='F')
		{
			read_fd_line(&line[2]);
		}
		else if (line[0]=='M')
		{
			read_map_line(&line[2]);
		}
	}
	lite_assert(!broken);
}

void MmapDecoder::read_fd_line(const char* line)
{
	uint32_t fd;
	char* fn;
	int rc = sscanf(line, "%x %as", &fd, &fn);
	lite_assert(rc==2);
	lite_assert(fd<MAX_FD_TABLE);	// too lazy to build a dynamic structure
	if (fd_table[fd]!=NULL)
	{
		free(fd_table[fd]);
	}
	fd_table[fd] = strdup(fn);
	free(fn);
}

void MmapDecoder::read_map_line(const char* line)
{
	uint32_t fd;
	uint32_t start, end, offset;
	int rc = sscanf(line, "%x %x %x %x",
		&fd, &start, &end, &offset);
	lite_assert(rc==4);
	lite_assert(fd<MAX_FD_TABLE);
	lite_assert(fd_table[fd]!=NULL);

	uint32_t size = end-start;

	// round fast_mmap size to page boundary, since client fast_mmap will
	// need to do the same thing to exploit this fast region.
	size = ((size-1)&(~0xfff))+0x1000;
	// TODO this is a source of waste; account for it.

/*
	if (strcmp(fd_table[fd], "/usr/lib/mozilla/plugins/librhythmbox-itms-detection-plugin.so")==0)
	{
		fprintf(stderr, "break here\n");
		fprintf(stderr, "yeah\n");
	}
*/

	MmapBehavior* behavior = query_file(fd_table[fd], true);
	if (offset==0)
	{
		if (behavior->aligned_mapping_copies==0)
		{
			behavior->aligned_mapping_size = size;
		}

		if (behavior->aligned_mapping_size != size)
		{
			// already have seen one 0-offset mapping, but it doesn't match
			// I've seen this happen when the second instance is actually
			// the data segment, but the text segment was so small that
			// the data segment fit inside the first page of the file.
			// (e.g. librhythmbox-itms-detection-plugin.so).
			// In this case, the second instance should be a 'precious'
			// range anyway; it can't get mmap_fast'd due to how the
			// client will request it.
			lite_assert(size <= 8192);	// just be sure we're not getting carried away and falling for this trick too often.
			MDRange this_range(offset, offset+size);
			behavior->insert_range(&this_range);
		}
		else
		{
			behavior->aligned_mapping_copies += 1;
		}
	}
	else
	{
		MDRange this_range(offset, offset+size);
		behavior->insert_range(&this_range);
	}
}

MmapBehavior* MmapDecoder::query_file(const char* filename, bool create)
{
	MmapBehavior key;
	key.filename = filename;
	MmapBehavior* result = (MmapBehavior*) hash_table_lookup(&ht, &key);
	if (result==NULL && create)
	{
		result = new MmapBehavior();
		result->filename = strdup(filename);
		hash_table_insert(&ht, result);
	}
	return result;
}

void MmapDecoder::_dbg_dump_foreach(void* v_this, void* v_behavior)
{
	MmapBehavior* behavior = (MmapBehavior*) v_behavior;
	fprintf(stderr, "%s: aligned_mapping_size %x x %d\n",
		behavior->filename,
		behavior->aligned_mapping_size,
		behavior->aligned_mapping_copies);
	
	LinkedListIterator lli;
	for (ll_start(&behavior->precious_ranges, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		MDRange* range = (MDRange*) ll_read(&lli);
		fprintf(stderr, "  range %x--%x\n", range->start, range->end);
	}
}

void MmapDecoder::dbg_dump()
{
	fprintf(stderr, "MmapDecoder has info on %d paths:\n", ht.count);
	hash_table_visit_every(&ht, _dbg_dump_foreach, this);
}
