#include "HandyDirRec.h"

HandyDirRec::HandyDirRec(MallocFactory *mf, ZFTPDirectoryRecord *drec, const char *name)
	: mf(mf),
	  name(mf_strdup(mf, name))
{
	this->drec = (ZFTPDirectoryRecord*) mf_malloc(mf, sizeof(*drec));
	*this->drec = *drec;
}

HandyDirRec::HandyDirRec(const char *name)
	: mf(NULL),
	  drec(NULL),
	  name((char *) name)
{}

HandyDirRec::~HandyDirRec()
{
	if (mf) {
		mf_free(mf, drec);
		mf_free(mf, name);
	}
}

void HandyDirRec::write(void *dest)
{
	drec->reclen = get_record_size();
	memcpy(dest, drec, sizeof(*drec));
	memcpy(((uint8_t*) dest)+sizeof(*drec), name, lite_strlen(name)+1);
}
