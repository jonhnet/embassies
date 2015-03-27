#pragma once

#include "pal_abi/pal_abi.h"
#include "zoog_malloc_factory.h"
#include "xax_skinny_network.h"

// We *have* to share a xax_skinny_network, so we might as well also
// share its prerequisites.
class ZBCContext {
public:
	ZBCContext(ZoogDispatchTable_v1 *zdt);

	ZoogDispatchTable_v1 *zdt;
	ZoogMallocFactory zmf;
	MallocFactory *mf;
	XaxSkinnyNetwork *xsn;
};

