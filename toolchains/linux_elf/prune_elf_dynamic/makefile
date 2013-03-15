
########## Variables

include ../make.arch

ZOOG_ROOT=../../..

#TEST_LOADER_PATH=$(ZOOG_ROOT)/toolchains/linux_elf/lib_links/ld-linux.so.2
# Can't test against zoog ld.so anymore; it's no longer compatible with
# linux ABI due to set_thread_area hack (static_rewriter/hack_patch_init_tls.c).
ifeq ($(ARCH),i386)
TEST_LOADER_PATH=/lib/ld-linux.so.2
else
ifeq ($(ARCH),x64)
TEST_LOADER_PATH=/lib/ld-linux-x86-64.so.2
else
TEST_LOADER_PATH=arch undefined
endif
endif

IFC_MODULE=$(ZOOG_ROOT)/common/ifc

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	LiteLib.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELFOBJ_MODULE) $(IFC_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELFOBJ_OBJS)

TARGETS=prune_elf_dynamic pie_init_twice_test

SRCS=\
	prune_elf_dynamic.c \

DEP_SRCS=$(wildcard *.c)

OBJS=$(patsubst %.o,$(BUILD)/%.o,$(patsubst %.c,%.o,$(SRCS)) $(MODULE_OBJS))

INCS=\
	-I../common

CFLAGS=$(INCS) \
	-fPIC \
	-DELFOBJ_USE_LIBC=1 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \

########## Include

include ../make.in

########## Rules

$(BUILD)/prune_elf_dynamic: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/pie_init_twice: $(BUILD)/pie_init_twice.o
	$(CC) -pie $(CFLAGS) $^ -o $@

$(BUILD)/pie_init_twice_noinit: $(BUILD)/pie_init_twice $(BUILD)/prune_elf_dynamic
	$(BUILD)/prune_elf_dynamic $< $@

$(BUILD)/pie_init_twice_test: $(BUILD)/pie_init_twice_noinit
	$(TEST_LOADER_PATH) $(BUILD)/pie_init_twice_noinit
.PHONY: $(BUILD)/pie_init_twice_test
