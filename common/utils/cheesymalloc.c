/*
#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include <sys/mman.h>
*/

#include <cheesymalloc.h>
#include "lite_log2.h"

#if MALLOC_TRACE_CHEESY
#include <malloc_trace.h>
#endif // MALLOC_TRACE_CHEESY

#include "LiteLib.h"

#define MALLOC_GUARD0	0x6d616c6c
#define MALLOC_GUARD1	0x6f636973
#define MALLOC_GUARD2	0x64656164
#define MALLOC_EMPTY	0xfe

#define CM_PAGE_MASK ((1<<12)-1)
#define ROUND_UP_PAGES(l)	(((l)+CM_PAGE_MASK) & (~CM_PAGE_MASK));
#define CM_INVALID_CALLER_LENGTH	(0xffffffff)

#define GUARD_CHECKS 1
#define DEBUG_SLOW 0
//#define STRACE_ON (g_state->config->strace)
#define STRACE_ON 1
	// turns out that stracing in here is only useful (to cheesy-accountant.py,
	// in particular) if it crosses all
	// contexts, since memory allocations get passed between contexts
	// (STRACE is only used here when DEBUG_SLOW is true.)

#if DEBUG_SLOW
#include <xax_interpose_tools.h>
extern XfsState *g_state;
#endif // DEBUG_SLOW

#define SANE_ALLOCATION_SIZE	0x20000000

void cheesy_malloc_init_arena(CheesyMallocArena *arena, AbstractMmapIfc *mmapifc)
{
	int bin;
	arena->mmapifc = mmapifc;

	cheesy_lock_init(&arena->mutex);	
	for (bin=0; bin<CM_NUM_BINS; bin++)
	{
		arena->binHead[bin].next = NULL;
	}

	arena->accounting.bytes_mmapped = 0;
	arena->accounting.bytes_mmap_fragmented = 0;
	arena->accounting.bytes_in_free_blocks = 0;
	arena->accounting.bytes_header = 0;
	arena->accounting.bytes_block_fragmented = 0;
	arena->accounting.bytes_allocated = 0;
}

uint32_t dbg_cheesy_count_free_list(CheesyMallocArena *arena)
{
	uint32_t count = 0;
	uint32_t members = 0;
	int bin;
	for (bin=0; bin<CM_NUM_BINS; bin++)
	{
		uint32_t bincount = 0;
		uint32_t bin_members = 0;
		CheesyMallocBlock *cmb;
		for (cmb = arena->binHead[bin].next; cmb!=NULL; cmb=cmb->next)
		{
			bincount += cmb->length;
			bin_members += 1;
		}

#if DEBUG_SLOW
		if (STRACE_ON)
		{
			char buf[512], hexbuf[16];
			strcpy(buf, "bin ");
			strcat(buf, hexlong(hexbuf, bin));
			strcat(buf, " members=");
			strcat(buf, hexlong(hexbuf, bin_members));
			strcat(buf, " size=");
			strcat(buf, hexlong(hexbuf, bincount));
			strcat(buf, "\n");
			debug_logfile_append("strace", buf);
		}
#endif // DEBUG_SLOW
		count += bincount;
		members += bin_members;
	}
#if DEBUG_SLOW
	if (STRACE_ON)
	{
		char buf[512], hexbuf[16];
		strcpy(buf, "total ");
		strcat(buf, " members=");
		strcat(buf, hexlong(hexbuf, members));
		strcat(buf, " size=");
		strcat(buf, hexlong(hexbuf, count));
		strcat(buf, "\n");
		debug_logfile_append("strace", buf);
	}
#endif // DEBUG_SLOW
	return count;
}

void _cheesy_insert_free_list(CheesyMallocArena *arena, int bin, CheesyMallocBlock *cmb)
{
	CheesyMallocBlock *binHead = NULL;
	// assumes caller holds arena->mutex
	lite_assert((uint32_t)((1<<bin)-1) == cmb->length);
	binHead = &arena->binHead[bin];
	lite_assert((char)cmb->is_free);
	cmb->next = binHead->next;
	binHead->next = cmb;
	arena->accounting.bytes_in_free_blocks += cmb->length;
}

CheesyMallocBlock *_cheesy_create_space(CheesyMallocArena *arena, uint32_t caller_length, int bin)
{
	// assumes caller holds arena->mutex
	uint32_t req_length;
	void *memory = NULL;
	uint32_t mmap_length = 0;

	if (bin < CM_NUM_BINS)
	{
		// reusable-sized requests:
		// allocate exactly the largest size that fits in the bin;
		// that way (a) it's big enough for any request from this bin,
		// and (b) when we free it, we'll put it back in the correct bin
		// (not the next bin up, where it would only be able to serve
		// some requests).
		req_length = (1<<bin)-1;
	}
	else
	{
		// big requests, which get unmapped: get only what we need.
		req_length = caller_length;
		lite_assert(caller_length != CM_INVALID_CALLER_LENGTH);
	}

	lite_assert(req_length < SANE_ALLOCATION_SIZE);
	mmap_length = ROUND_UP_PAGES(req_length);
	memory = (arena->mmapifc->mmap_f)(arena->mmapifc, mmap_length);
	lite_assert(mmap_length >= req_length);
	// Verify we got the region size we expected
	((char*)memory)[0] = 7;  // Cheap assert-like test
	((char*)memory)[mmap_length-1] = 8;  // Cheap assert-like test
#if DEBUG_SLOW
		if (STRACE_ON)
		{
			char buf[512], hexbuf[16];
			strcpy(buf, "_cm:mmap: addr=");
			strcat(buf, hexlong(hexbuf, (uint32_t) memory));
			strcat(buf, " size=");
			strcat(buf, hexlong(hexbuf, mmap_length));
			strcat(buf, "\n");
			debug_logfile_append("strace", buf);
		}
#endif // DEBUG_SLOW
	arena->accounting.bytes_mmapped += mmap_length;

	{
	uint32_t mmap_fragmentation = 0;
	void *memory_ptr = memory;
	void *memory_end = (char*)memory+mmap_length;
	CheesyMallocBlock *result = NULL;
	while ((char*)memory_ptr+req_length <= (char*)memory_end)
	{
		CheesyMallocBlock *cmb = (CheesyMallocBlock *) memory_ptr;
#if GUARD_CHECKS
		cmb->guard0 = MALLOC_GUARD0;
		cmb->guard1 = MALLOC_GUARD1;
#endif // GUARD_CHECKS
		cmb->length = req_length;
		cmb->is_free = 1;
		cmb->acct_user_len = 0;
		cmb->arena = arena;
		if (result==NULL)
		{
			// first block set aside for caller
			// This way we avoid inserting big blocks onto free lists,
			// so we needn't reason about them ever coming off of a free
			// list into the wrong hands. (They wouldn't, but that requires
			// a less-obvious temporal invariant.)
			result = cmb;
		}
		else
		{
			if (bin < CM_NUM_BINS)
				// NB this if is just here to point out to the compiler
				// that we're not accessing past the end of the array
				// bounds.
			{
				_cheesy_insert_free_list(arena, bin, cmb);
			}
			else
			{
				lite_assert(false);
			}
		}
		memory_ptr = (char*) memory_ptr + req_length;

		// since small req_lengths are 2^n-1, add 1 to stay
		// on 2^k alignment boundary.
		memory_ptr = (char*) memory_ptr + 1;
		mmap_fragmentation += 1;
	}
	mmap_fragmentation += ((char*)memory_end - (char*)memory_ptr);
	arena->accounting.bytes_mmap_fragmented += mmap_fragmentation;
	return result;
	}
}

void _cheesy_check_accounting(CheesyMallocArena *arena)
{
	// assumes caller holds arena->mutex
	CMAccounting *acct = &arena->accounting;
	uint32_t usage =
		acct->bytes_mmap_fragmented +
		acct->bytes_in_free_blocks +
		acct->bytes_header +
		acct->bytes_block_fragmented +
		acct->bytes_allocated;
	lite_assert( usage == acct->bytes_mmapped );
}

void *cheesy_malloc(CheesyMallocArena *arena, uint32_t req_length)
{
	uint32_t overhead_len = 0;
	int bin = 0;
	CheesyMallocBlock *cmb = NULL;
	void *result = NULL;

	if (req_length == 4096) {
#if WINDOWS_bryan_what_symbol_would_you_like_here_question_mark
		__asm {
			int 3
		}
#endif // WINDOWS
	}

	cheesy_lock_acquire(&arena->mutex);
	overhead_len = (req_length + sizeof(CheesyMallocBlock) + sizeof(CheesyMallocTail));
	bin = lite_log2(overhead_len);
	
	if (bin < CM_NUM_BINS)
	{
		if (arena->binHead[bin].next!=NULL)
		{
			cmb = arena->binHead[bin].next;
			arena->binHead[bin].next = cmb->next;
			cmb->next = NULL;
			arena->accounting.bytes_in_free_blocks -= cmb->length;
		}
		else
		{
			cmb = _cheesy_create_space(arena, CM_INVALID_CALLER_LENGTH, bin);
		}
	}
	else
	{
		cmb = _cheesy_create_space(arena, overhead_len, bin);
	}
	arena->accounting.bytes_block_fragmented += (cmb->length - overhead_len);
	arena->accounting.bytes_header += sizeof(CheesyMallocBlock) + sizeof(CheesyMallocTail);
	arena->accounting.bytes_allocated += req_length;

	_cheesy_check_accounting(arena);

// Now that we've got a CMB to ourselves, we release the lock
// and do local accounting and (debug) memory-clearing without
// creating lock contention.
	cheesy_lock_release(&arena->mutex);

	lite_assert(cmb!=NULL);

	lite_assert (overhead_len <= cmb->length);
	lite_assert((char)cmb->is_free);

	cmb->is_free = 0;
	lite_assert(cmb->length < SANE_ALLOCATION_SIZE);
	result = (((char*) cmb)+sizeof(CheesyMallocBlock));

	cmb->acct_user_len = req_length;

#if GUARD_CHECKS
	lite_assert(cmb->guard0==MALLOC_GUARD0
		&& cmb->guard1==MALLOC_GUARD1);

	{
	CheesyMallocTail *tail = (CheesyMallocTail *)(((char*)cmb) + cmb->length - sizeof(CheesyMallocTail));
	tail->guard2 = MALLOC_GUARD2;
	}

#if DEBUG_MALLOC_EMPTY
	lite_memset(result, MALLOC_EMPTY,
		cmb->length - (sizeof(CheesyMallocBlock)+sizeof(CheesyMallocTail)));
#endif // DEBUG_MALLOC_EMPTY

#endif // GUARD_CHECKS

#if MALLOC_TRACE_CHEESY
	xi_trace_alloc(tl_cheesy, result, req_length);
#endif // MALLOC_TRACE_CHEESY

#if DEBUG_SLOW
	if (STRACE_ON)
	{
		char buf[512], hexbuf[16];
		strcpy(buf, ":cheesy_malloc@");
		strcat(buf, hexlong(hexbuf, (uint32_t) &cheesy_malloc));
		strcat(buf, "(");
		strcat(buf, hexlong(hexbuf, req_length));
		strcat(buf, ") = ");
		strcat(buf, hexlong(hexbuf, (uint32_t) cmb));
		strcat(buf, " len:");
		strcat(buf, hexlong(hexbuf, cmb->length));
		strcat(buf, "\n");
		debug_logfile_append("strace", buf);
	}
#endif // DEBUG_SLOW

	if (req_length>0)
	{
		// Verify we got the region size we expected
		//lite_assert(((char*)result)[0] && ((char*)result)[req_length-1]);
		((char*)result)[0] = 9; // Cheap assert-like check
		((char*)result)[req_length-1] = 5; // Cheap assert-like check
	}
	else
	{
		// um, probably oughtn't stomp space beyond what was allocated...
	}

	// let's just assert this here, to avoid failure deep elsewhere
	lite_assert(result!=NULL);

	return result;
}

void cheesy_free(void *ptr)
{
	int bin = 0;
#if MALLOC_TRACE_CHEESY
	xi_trace_free(tl_cheesy, ptr);
#endif // MALLOC_TRACE_CHEESY

	CheesyMallocBlock *cmb = (CheesyMallocBlock *)
		((char*)ptr - sizeof(CheesyMallocBlock));
#if GUARD_CHECKS
	lite_assert(cmb->guard0==MALLOC_GUARD0
		&& cmb->guard1==MALLOC_GUARD1
		&& cmb->is_free==0);
	{
	CheesyMallocTail *tail = (CheesyMallocTail *) (((char*)cmb) + cmb->length - sizeof(CheesyMallocTail));
	lite_assert(tail->guard2 == MALLOC_GUARD2);
	}
	lite_assert(cmb->length < SANE_ALLOCATION_SIZE);
	lite_assert(cmb->length-sizeof(CheesyMallocBlock) < SANE_ALLOCATION_SIZE);
#endif // GUARD_CHECKS

#if DEBUG_MALLOC_EMPTY
	lite_memset(ptr, MALLOC_EMPTY, cmb->length-sizeof(CheesyMallocBlock));
#endif // DEBUG_MALLOC_EMPTY

	cmb->is_free = 1;
	bin = lite_log2(cmb->length);

#if DEBUG_SLOW
	if (STRACE_ON)
	{
		char buf[512], hexbuf[16];
		strcpy(buf, ":cheesy_free@");
		strcat(buf, hexlong(hexbuf, (uint32_t) &cheesy_malloc));
		strcat(buf, "(");
		strcat(buf, hexlong(hexbuf, (uint32_t) cmb));
		strcat(buf, " len:");
		strcat(buf, hexlong(hexbuf, cmb->length));
		strcat(buf, ")\n");
		debug_logfile_append("strace", buf);
	}
#endif // DEBUG_SLOW
	
	{
	void *memaddr = NULL;
	CheesyMallocArena *arena = cmb->arena;
	cheesy_lock_acquire(&arena->mutex);
#define HEADER_SIZE	(sizeof(CheesyMallocBlock) + sizeof(CheesyMallocTail))
	arena->accounting.bytes_header -= HEADER_SIZE;
	arena->accounting.bytes_allocated -= cmb->acct_user_len;
	arena->accounting.bytes_block_fragmented -= (cmb->length - cmb->acct_user_len - HEADER_SIZE);
	cmb->acct_user_len = 0;
	// these bytes we just decremented sum to cmb->length;
	// they'll either be added to free_list or subtracted from mmap_length.

	// NB: There's a slightly perilous invariant that shows that the
	// following test (bin >= CM_NUM_BINS) is the right one.
	// We need to be sure that we never put something on the free list
	// that was allocated just-big-enough instead of bin-max-size.
	// The test in cheesy_malloc is on cm_log2(overhead_len); and in that
	// case, overhead_len is what gets written into cmb->length, which
	// is the value we apply to cm_log2() above. Thus, the tests are
	// on consistent inputs, and should generate consistent outputs.
	// This invariant is verified in the first assert.
	if (bin < CM_NUM_BINS)
	{
		lite_assert(cmb->length == (uint32_t)((1<<bin)-1));
		// verify the "perilous invariant" described above.

		// small allocation: hang onto it and reuse it.
		// TODO: this can waste a lot of space if the bucket
		// has a high high-water mark. Should be looking to allocate
		// from busy buckets, encouraging other buckets to drain empty.
		_cheesy_insert_free_list(arena, bin, cmb);
	}
	else
	{
		// similarly to the "perilous invariant", we need to
		// be sure we munmap the same length we mmap()ed.
		// Again, we do so by using the same expression in both
		// cases: mmap used ROUND_UP_PAGES(req_length), which was
		// then stored in cmb->length for use here.
		uint32_t mmap_length = ROUND_UP_PAGES(cmb->length);
		arena->accounting.bytes_mmapped -= mmap_length;
		arena->accounting.bytes_mmap_fragmented -= (mmap_length - cmb->length);

		// big allocation: give it back to system.
		memaddr = (void*) cmb;
		//lite_assert((((uint32_t) memaddr) & CM_PAGE_MASK)==0); // Deprecated
		(arena->mmapifc->munmap_f)(arena->mmapifc, memaddr, mmap_length);
#if DEBUG_SLOW
		if (STRACE_ON)
		{
			char buf[512], hexbuf[16];
			strcpy(buf, "_cm:munmap: addr=");
			strcat(buf, hexlong(hexbuf, (uint32_t) memaddr));
			strcat(buf, " size=");
			strcat(buf, hexlong(hexbuf, mmap_length));
			strcat(buf, "\n");
			debug_logfile_append("strace", buf);
		}
#endif // DEBUG_SLOW
	}


	_cheesy_check_accounting(arena);

	cheesy_lock_release(&arena->mutex);
	}
}
