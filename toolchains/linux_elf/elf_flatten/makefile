
########## Variables

include ../make.arch


ZOOG_ROOT=../../..

TARGETS=elf_flatten check_section_shifting

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
COMMON_UTILS_MODULE_OBJS=\

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	LiteLib.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE) $(ELFOBJ_MODULE)
MODULE_OBJS=$(COMMON_UTILS_MODULE_OBJS) $(ELF_COMMON_OBJS) $(ELFOBJ_OBJS)

EF_SRCS=\
	elf_flatten.c \
	elf_flatten_args.c \

CSS_SRCS=\
	check_section_shifting.c \

DEP_SRCS=$(EF_SRCS) $(CSS_SRCS)

EF_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(EF_SRCS) $(MODULE_OBJS)))
CSS_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(CSS_SRCS) $(MODULE_OBJS)))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=\
	-g \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-DZLC_USE_PRINTF=1 \
	-DELFOBJ_USE_LIBC=1 \

########## Include

include ../make.in

########## Rules


$(BUILD)/elf_flatten: $(EF_OBJS)
	$(CC) $^ -g -o $@ -I../common

$(BUILD)/check_section_shifting: $(CSS_OBJS)
	$(CC) $^ -g -o $@ -I../common
