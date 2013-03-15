########## Variables

ZOOG_ROOT=../../..

TARGETS=rx_test tx_test

DEP_SRCS=$(wildcard *.c)

SRCS=\

LIB_COMMON_MODULE=$(ZOOG_ROOT)/common/lib
LIB_COMMON_OBJS=\
#	LiteLib.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
#	debug.o \
#	debug_util.o \

ZFTP_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/zftp_module/
ZFTP_OBJS=\
#	zftp_util.o \
#	zftp_readers.o \

MODULES=$(LIB_COMMON_MODULE) $(ELF_COMMON_MODULE) $(ZFTP_MODULE)
MODULE_OBJS=$(LIB_COMMON_OBJS) $(ELF_COMMON_OBJS) $(ZFTP_OBJS)

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=\
	-g \
	$(INCS) \
	-DSTANDALONE=1 \
	-DZFTP_USE_OPENSSL=1 \

#	-DDEBUG=1 \

LIBFLAGS=\
	-lssl

########## Include

include ../make.in

########## Rules

$(BUILD)/rx_test: $(OBJS) rx_test.o
	$(CC) $(LIBFLAGS) $^ -o $@

$(BUILD)/tx_test: $(OBJS) tx_test.o
	$(CC) $(LIBFLAGS) $^ -o $@
