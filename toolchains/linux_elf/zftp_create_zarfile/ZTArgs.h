/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "args_lib.h"

// --zarfile <zarfile_foo>
// | --list true
// | --extract <path>

class ZTArgs {
public:
	const char *zarfile;
	const char *extract_path;
	const char *dir_path;
	const char *flat_unpack_dest;
	bool verbose;
	enum Mode {
		unset,
		extract,
		list_files,
		list_chunks,
		list_dir,
		flat_unpack,
	};
	Mode mode;

public:
	ZTArgs(int argc, char **argv);
};
