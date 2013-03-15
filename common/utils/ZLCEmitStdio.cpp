#include <string.h>
#include <stdio.h>
#include "ZLCEmitStdio.h"

ZLCEmitStdio::ZLCEmitStdio(FILE *fp, ZLCChattiness threshhold)
{
	this->fp = fp;
	this->_threshhold = threshhold;
}

void ZLCEmitStdio::emit(const char *s)
{
	fwrite(s, strlen(s), 1, fp);
	fflush(fp);
}
