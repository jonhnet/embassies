########## Variables

ZOOG_ROOT=../../..

include ../make.modules

TARGETS=posix_perf_measure

PPM_SRCS=\
	posix_perf_measure.cpp \

DEP_SRCS=$(ZTP_SRCS) $(ZTH_SRCS) $(ZLT_SRCS)

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	malloc_factory.o \
	LiteLib.o \
	malloc_trace.o \
	linked_list.o \
	hash_table.o \
	xax_network_utils.o \
	lite_log2.o \
	cheesy_snprintf.o \
	ambient_malloc.o \
	TunIDAllocator.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	xe_mark_perf_point.o \

ZFTP_OBJS=\
	$(ZFTP_CORE_OBJS) \
	$(ZFTP_SERVER_OBJS) \

#MODULES=$(COMMON_UTILS_MODULE)  $(ZFTP_MODULE) $(CRYPTO_MODULE)
#MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS) $(ZFTP_OBJS) $(CRYPTO_MODULE_OBJS)
MODULES=$(ELF_COMMON_MODULE)
MODULE_OBJS=$(ELF_COMMON_OBJS)

PPM_OBJS=$(call mapSrcsToObjs,$(PPM_SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \

CFLAGS=\
	-g \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

#	-DDEBUG=1 \

LIBFLAGS=\
	-lpthread \
	-ldb \
	-lz \

########## Include

include ../make.in

########## Rules

$(BUILD)/posix_perf_measure: $(PPM_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)
