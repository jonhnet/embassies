#include "math_util.h"
#include "paltest.h"

void
allocfreetest(
	int iterations)
{
	(g_debuglog)("stderr", "Beginning memory allocation test.\n");
	// jonh dials memtest max size down from 1GB to 256MB, as bigger
	// sizes choke our L4 implementations.
	const size_t max_length = 1 << 28;
	int iter = 0;
	for (iter = 0; iter < iterations; iter++)
	{
		int length = 0;
		for (length = 1; length <= max_length; length <<= 1)
		{
			(g_debuglog)("stderr", ".");
			void *memory_region = (g_zdt->zoog_allocate_memory)(length);
			if (memory_region == 0)
			{
				(g_debuglog)("stderr", "zoog_allocate_memory() returned null pointer.\n");
				(g_zdt->zoog_exit)();
			}

			// try writing a few spots in the allocated memory
			uint8_t *ptr = (uint8_t*) memory_region;
			int i;
			// check the opening region for zeros carefully, so that
			// we don't just miss some 7's due to stride misalignment.
			for (i=0; i<length && i<8192; i++)
			{
				if (ptr[i] != 0)
				{
					(g_debuglog)("stderr", "zoog_allocate_memory() returned non-zeroed memory.\n");
					(g_zdt->zoog_exit)();
				}
			}
			for (i=0; i<length; i+=max(length>>3, 1))
			{
				if (ptr[i] != 0)
				{
					(g_debuglog)("stderr", "zoog_allocate_memory() returned non-zeroed memory.\n");
					(g_zdt->zoog_exit)();
				}
				ptr[i] = 7;
			}
			ptr[length-1] = 7;

			(g_zdt->zoog_free_memory)(memory_region, length);
		}
		(g_debuglog)("stderr", "\n");
	}
}

void
readwritetest(
	size_t length,
	int iterations)
{
	(g_debuglog)("stderr", "Beginning memory read/write test.\n");
	const unsigned int dots = 16;
	unsigned int *memory_region = (unsigned int *)((g_zdt->zoog_allocate_memory)(length));
	if (memory_region == 0)
	{
		(g_debuglog)("stderr", "zoog_allocate_memory() returned null pointer.\n");
		(g_zdt->zoog_exit)();
	}
	unsigned int locations = length / sizeof(unsigned int);
	unsigned int iter = 0;
	for (iter = 0; iter < iterations; iter++)
	{
		unsigned int mask = 0x01010101 << iter;
		unsigned int loc = 0;
		for (loc = 0; loc < locations; loc++)
		{
			if (loc % (locations / dots) == 0)
			{
				(g_debuglog)("stderr", "+");
			}
			memory_region[loc] = loc * mask;
		}
		for (loc = 0; loc < locations; loc++)
		{
			if (loc % (locations / dots) == 0)
			{
				(g_debuglog)("stderr", "-");
			}
			if (memory_region[loc] != loc * mask)
			{
				(g_debuglog)("stderr", "Written and read values disagree.\n");
				(g_zdt->zoog_exit)();
			}
		}
		(g_debuglog)("stderr", "\n");
	}
	(g_zdt->zoog_free_memory)((void *)memory_region, length);
}

void
overlaptest(
	size_t min_length,
	size_t max_length,
	int regions)
{
	(g_debuglog)("stderr", "Beginning memory overlap test.\n");
	size_t *lengths = (g_zdt->zoog_allocate_memory)(sizeof(size_t) * regions);
	void **pointers = (g_zdt->zoog_allocate_memory)(sizeof(void *) * regions);
	unsigned int region = 0;
	for (region = 0; region < regions; region++)
	{
		lengths[region] = 0;
		pointers[region] = 0;
	}
	size_t length = min_length;
	for (region = 0; region < regions; region++)
	{
		lengths[region] = length;
		pointers[region] = (g_zdt->zoog_allocate_memory)(length);
		unsigned int prev_region = 0;
		for (prev_region = 0; prev_region < region; prev_region++)
		{
			(g_debuglog)("stderr", ".");
			if (pointers[region] + lengths[region] > pointers[prev_region] &&
				pointers[region] < pointers[prev_region] + lengths[prev_region])
			{
				(g_debuglog)("stderr", "Two allocated regions overlap.\n");
			}
		}
		(g_debuglog)("stderr", "\n");
		length <<= 1;
 		if (length > max_length)
		{
			length = min_length;
		}
	}
	for (region = 0; region < regions; region++)
	{
		(g_zdt->zoog_free_memory)(pointers[region], lengths[region]);
		pointers[region] = 0;
	}
	(g_zdt->zoog_free_memory)(lengths, sizeof(size_t) * regions);
	(g_zdt->zoog_free_memory)(pointers, sizeof(void *) * regions);
}

void
memtest()
{
	allocfreetest(8);
	readwritetest(4096, 8);
	overlaptest(16, 1048576, 12);
}

