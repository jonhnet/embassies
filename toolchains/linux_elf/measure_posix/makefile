
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	measure_posix \

MP_SRCS=\
	measure_posix.c \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_MODULE_OBJS=\
	LiteLib.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_MODULE_OBJS=\
	ElfLoad.o \
	ElfArgs.o \
	xe_mark_perf_point.o \

MODULES=$(ELF_COMMON_MODULE) $(COMMON_UTILS_MODULE)
MODULE_OBJS=$(ELF_COMMON_MODULE_OBJS) $(COMMON_UTILS_MODULE_OBJS)

MP_OBJS=$(call mapSrcsToObjs, $(MP_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(MP_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DUSE_FILE_IO=1 \


########## Include

include ../make.in

########## Rules

$(BUILD)/measure_posix: $(MP_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

