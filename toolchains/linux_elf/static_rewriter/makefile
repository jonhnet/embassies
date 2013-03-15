
########## Variables

include ../make.arch

ZOOG_ROOT=../../..

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
COMMON_UTILS_MODULE_OBJS=\
	LiteLib.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_MODULE_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

MODULES=$(ELFOBJ_MODULE) $(COMMON_UTILS_MODULE)
MODULE_OBJS=$(ELFOBJ_MODULE_OBJS) $(COMMON_UTILS_MODULE_OBJS)

TARGETS=static_rewriter segreg_scanner

SRCS=\
	add_segment.c \
	disasm.c \
	functable.c \
	jump_target_list.c \
	string_list.c \
	new_sections.c \
	hack_patch_init_tls.c \

DEP_SRCS=$(wildcard *.c)

OBJS=$(patsubst %.o,$(BUILD)/%.o,$(patsubst %.c,%.o,$(SRCS)) $(MODULE_OBJS))

INCS=\
	-I../common \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=$(INCS) \
	-DELFOBJ_USE_LIBC=1 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \

########## Include

include ../make.in

########## Rules

$(BUILD)/static_rewriter: $(OBJS) $(BUILD)/static_rewriter.o $(BUILD)/static_rewriter_args.o
	$(CC) $(CFLAGS) $^ -o $@ -ldisasm

$(BUILD)/segreg_scanner: $(OBJS) $(BUILD)/segreg_scanner.o $(BUILD)/segreg_args.o
	$(CC) $(CFLAGS) $^ -o $@ -ldisasm

$(BUILD)/disasm.o: /usr/include/libdis.h
/usr/include/libdis.h:
	sudo apt-get install libdisasm-dev
