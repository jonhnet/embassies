#pragma once

#include "malloc_factory.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	UNSET_PLACEMENT,
	GREEDY_PLACEMENT,
	STABLE_PLACEMENT,
} PlacementAlgorithmEnum;

typedef struct {
	char *trace;
	char *zar;
	bool include_elf_syms;
	bool verbose;
    bool hide_icons_dir;
	LinkedList overlay_list;
	PlacementAlgorithmEnum placement_algorithm_enum;
	uint32_t zftp_lg_block_size;
	bool pack_small_files;
} ZCZArgs;

void args_init(ZCZArgs *args, int argc, char **argv, MallocFactory *mf);

#ifdef __cplusplus
}
#endif
