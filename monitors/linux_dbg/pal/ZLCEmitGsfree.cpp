#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>

#include "LiteLib.h"
#include "ZLCEmitGsfree.h"
#include "gsfree_syscall.h"

ZLCEmitGsfree::ZLCEmitGsfree(ZLCChattiness threshhold)
{
	this->_threshhold = threshhold;
}

void ZLCEmitGsfree::emit(const char *s)
{
	_gsfree_syscall(__NR_write, 2, s, lite_strlen(s));
}
