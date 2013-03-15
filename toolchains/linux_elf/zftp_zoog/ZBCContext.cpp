#include "ZBCContext.h"
#include "SyncFactory_Zutex.h"

ZBCContext::ZBCContext(ZoogDispatchTable_v1 *zdt)
{
	this->zdt = zdt;
	zoog_malloc_factory_init(&zmf, zdt);
	this->mf = zmf_get_mf(&zmf);
	this->xsn = new XaxSkinnyNetwork(zdt, mf);
}
