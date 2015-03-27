#pragma once

#include "DataRange.h"
#include "FileOperationIfc.h"
#include "FileScanner.h"

class Blaminator : public FileOperationIfc {
private:
	DataRange range;
public:
	Blaminator(DataRange range) : range(range) {}
	void operate_zhdr(ZF_Zhdr *zhdr);
	void operate(FileScanner *scanner);
	int result();
};

