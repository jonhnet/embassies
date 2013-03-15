#include "LiteLib.h"
#include "reference_file.h"

ReferenceFile *reference_file_init(MallocFactory *mf, uint32_t pattern_selector)
{
	ReferenceFile *rf = (ReferenceFile *) mf_malloc(mf, sizeof(ReferenceFile));
	rf->mf = mf;
	rf->pattern_selector = pattern_selector;
	rf->len =  (1<<(pattern_selector>>2)) + (pattern_selector&3) - 1;
	rf->bytes = (uint8_t*) mf_malloc(mf, rf->len);
	lite_memset(rf->bytes, (pattern_selector-3) & 0xff, rf->len);
	return rf;
}

void reference_file_free(ReferenceFile *rf)
{
	mf_free(rf->mf, rf->bytes);
	mf_free(rf->mf, rf);
}
