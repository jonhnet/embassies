#pragma once

#include "zarfile.h"

class FileScanner;

class FileOperationIfc {
public:
	virtual void operate_zhdr(ZF_Zhdr *zhdr) {}
	virtual void operate(FileScanner *scanner) = 0;
	virtual int result() = 0;
};
