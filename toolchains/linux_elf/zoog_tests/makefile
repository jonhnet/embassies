
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	zoog_identity_test \
	ulrich_test \
	cpuid_test \

ZIT_SRCS=\
	zoog_identity_test.cpp \

UT_SRCS=\
	ulrich_test.c \

CPUT_SRCS=\
	cpuid_test.c \

include ../make.modules

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
  ambient_malloc.o \
  malloc_factory.o \
  LiteLib.o \
  standard_malloc_factory.o \


CRYPTO_MODULE_OBJS+= \
	time_posix.o \
	timer.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	get_xax_dispatch_table.o \
	AppIdentity.o \

MODULES= $(CRYPTO_MODULE) $(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE)
MODULE_OBJS= $(CRYPTO_MODULE_OBJS) $(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS)

ZIT_OBJS=$(call mapSrcsToObjs, $(ZIT_SRCS) $(MODULE_OBJS))

UT_OBJS=$(call mapSrcsToObjs, $(UT_SRCS) $(MODULE_OBJS))

CPUT_OBJS=$(call mapSrcsToObjs, $(CPUT_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(ZIT_SRCS) $(UT_SRCS) $(CPUT_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \

CFLAGS=\
	-g -pie -fpie \
	-march=native \
	-O3 \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \


########## Include

include ../make.in

########## Rules

$(BUILD)/zoog_identity_test: $(ZIT_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

$(BUILD)/ulrich_test: $(UT_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@ -lpthread

$(BUILD)/cpu_test: $(CPUT_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@ -lpthread

