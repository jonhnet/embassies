#pragma once

#include "args_lib.h"

// --zarfile <zarfile_foo>
// | --list true
// | --extract <path>

class PDArgs {
public:
	const char *file[2];

public:
	PDArgs(int argc, char **argv);
};
