#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>	// htonl

#include "elfobj.h"
#include "elf_flat_reader.h"
#include "zarfile.h"
#include "linked_list.h"
#include "standard_malloc_factory.h"
#include "copy_file.h"
#include "zcz_args.h"
#include "ZFSReader.h"
#include "math_util.h"
#include "LiteLib.h"
#include "SyncFactory_Pthreads.h"

#include "Catalog.h"

int main(int argc, char **argv)
{
	ZArgs zargs;
	args_init(&zargs, argc, argv, standard_malloc_factory_init());

	SyncFactory_Pthreads sf;
	Catalog catalog(&zargs, &sf);
	catalog.scan(zargs.log, zargs.dep_file, zargs.zar);
	catalog.emit(zargs.zar);

	return 0;
}
