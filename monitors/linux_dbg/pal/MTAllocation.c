#include "profiler_ifc.h"
#include "MTAllocation.h"

MTAllocation *collect_fingerprint(MallocFactory *mf, int depth, int skip, void *start_ebp)
{
	int size = sizeof(MTAllocation)+sizeof(uint32_t)*depth;
	MTAllocation *mta = (MTAllocation *)mf_malloc(mf, size);

	if (start_ebp==NULL)
	{
		asm(
			"mov	%%ebp,%0;"
			: "=m"(start_ebp)
		);
	}
	mta->depth = depth;
	int frame_i, pc_i;
	void *ebp = start_ebp;
	for (frame_i=0, pc_i=0; pc_i<depth; frame_i+=1)
	{
		// gather this frame info & advance ebp up the stack
		uint32_t pc_addr = 0;
		if (ebp != 0)
		{
			pc_addr = ((uint32_t*)ebp)[1];
			void *next_ebp = (void*) (((uint32_t*)ebp)[0]);

			if (((uint32_t)(next_ebp - ebp)) > 10<<10)
			{
				// interpret a 10-kb frame as nonsense, and give up.
				next_ebp = 0;
			}
#if 0
			if (pc_addr == 0xb7fbaf70)
			{
				// xinterpose(). Stack above here is a mess; can't
				// reconstruct without symbols.
				next_ebp = 0;
			}
#endif
			ebp = next_ebp;
		}

		if (frame_i >= skip)
		{
			// record
			mta->pc[pc_i] = pc_addr;
			pc_i+=1;
		}
	}
	return mta;
}

uint32_t fp_hashfn(const void *obj)
{
	MTAllocation *mta = (MTAllocation *) obj;
	uint32_t hash = 0;
	int i;
	for (i=0; i<mta->depth; i++)
	{
		hash = hash*131 + mta->pc[i];
	}
	return hash;
}

int fp_comparefn(const void *o0, const void *o1)
{
	MTAllocation *mta0 = (MTAllocation *) o0;
	MTAllocation *mta1 = (MTAllocation *) o1;

	_profiler_assert(mta0->depth == mta1->depth);
	int i;
	for (i=0; i<mta0->depth; i++)
	{
		int rc = mta0->pc[i] - mta1->pc[i];
		if (rc!=0)
		{
			return rc;
		}
	}
	return 0;
}

