#pragma once
#include <stdio.h>
#include "WriterIfc.h"

class StdioWriter : public WriterIfc
{
private:
	FILE* fp;
public:
	StdioWriter(FILE* fp);
	void write(uint8_t* data, int len);
};
