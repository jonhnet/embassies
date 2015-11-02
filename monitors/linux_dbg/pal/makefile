########## Variables

# When we get around to supporting another ARCH, we'll want to
# hoist this definition into an include file.
ARCH=i386

ZOOG_ROOT=../../..

TARGETS=xax_port_pal

# zutexercise

include $(ZOOG_ROOT)/toolchains/linux_elf/make.modules

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	hash_table.o \
	LiteLib.o \
	lite_log2.o \
	xax_util.o \
	cheesy_snprintf.o \
	c_qsort.o \
	zoog_qsort.o \
	malloc_factory_operator_new.o \
	pure_virtual.o \
	linked_list.o \
	SyncFactory_Zutex.o \
	ambient_malloc.o \
	TunIDAllocator.o \

CRYPTO_MODULE=$(ZOOG_ROOT)/common/crypto
CRYPTO_MODULE_OBJS=\
	bignum.o \
	rijndael.o \
	rsa.o \
	sha1.o \
	sha2big.o \
	sha2small.o \
	vmac.o \
	CryptoException.o \
	DomainName.o \
	RandomSupply.o \
	ZBinaryRecord.o \
	ZCert.o \
	ZCertChain.o \
	ZDelegationRecord.o \
	ZKey.o \
	ZKeyLinkRecord.o \
	ZKeyPair.o \
	ZPrivKey.o \
	ZPubKey.o \
	ZPubKeyRecord.o \
	ZRecord.o \
	ZSigRecord.o \
	crypto.o \
	ByteStream.o \
	time_posix.o \
	ZSymKey.o \
	PrfKey.o \
	MacKey.o \
	SymEncKey.o \
	KeyDerivationKey.o \

# TODO probably should promote these files up into COMMON_UTILS
ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	cheesylock.o \
	malloc_trace.o \
	cheesymalloc.o \
	cheesy_malloc_factory.o \
	zutex_sync.o \

MONITOR_COMMON_MODULE=$(ZOOG_ROOT)/monitors/common/
MONITOR_COMMON_OBJS=\
	CallCounts.o \

MONITOR_LINUX_DBG_COMMON_MODULE=$(ZOOG_ROOT)/monitors/linux_dbg/common

MODULES=\
	$(COMMON_UTILS_MODULE) \
	$(CRYPTO_MODULE) \
	$(ELF_COMMON_MODULE) \
	$(MONITOR_COMMON_MODULE) \
	$(MONITOR_LINUX_DBG_COMMON_MODULE) \

MODULE_OBJS=\
	$(COMMON_UTILS_OBJS) \
	$(CRYPTO_MODULE_OBJS) \
	$(ELF_COMMON_OBJS) \
	$(MONITOR_COMMON_OBJS) \

XAXPORTPAL_SRCS=\
	xax_port_pal.cpp \
	xaxInvokeLinux.cpp \
	gsfree_syscall.S \
	raw_clone.S \
	memtracker.c \
	profiler.c \
	xpbtable.cpp \
	gsfree_lib.c \
	ClockAlarmThread.cpp\
	MonitorAlarmThread.cpp\
	pal_thread.c \
	ZLCEmitGsfree.c \
	HostAlarms.cpp \
	SyncFactory_GsfreeFutex.cpp \
	MTAllocation.c \
	pal_profiler_ifc.c \

#ZUTEXERCISE_SRCS=\
#	zutexercise.cpp \
#	xaxInvokeLinux.cpp \
#	gsfree_syscall.c \
#	raw_clone.S \
#	memtracker.c \
#	profiler.c \
#	xpbtable.c \
#	gsfree_lib.c \
#	AlarmThread.cpp\
#	pal_thread.c \

DEP_SRCS=$(XAXPORTPAL_SRCS) $(ZUTEXERCISE_SRCS)

XAXPORTPAL_OBJS=$(call mapSrcsToObjs,$(XAXPORTPAL_SRCS) $(MODULE_OBJS))
ZUTEXERCISE_OBJS=$(call mapSrcsToObjs,$(ZUTEXERCISE_SRCS) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/monitors/linux_dbg/common \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \

CFLAGS=\
	-g \
	$(INCS) \

LIBS = \
	-L$(ZOOG_ROOT)/toolchains/linux_elf/lib_links/debug_links \
	-lcrypto \

ADJ_LINKER_SCRIPT=\
	$(ZOOG_ROOT)/toolchains/linux_elf/common/adjust_linker_script.py

########## Include

include ../make.in

########## Rules

# We relocate this binary, so that if another non-pie binary loads later on
# and splats itself at 0x08040000, the mmap will fail rather than
# splattering the pal and leading to a very confusing failure.
$(BUILD)/linker_script: $(ADJ_LINKER_SCRIPT)
	$(ADJ_LINKER_SCRIPT) 0x20000000 > $@

$(BUILD)/xax_port_pal: $(XAXPORTPAL_OBJS) $(BUILD)/linker_script
	$(CXX) -static $(XAXPORTPAL_OBJS) -T$(BUILD)/linker_script -o $@ $(LIBS)

#$(BUILD)/zutexercise: $(ZUTEXERCISE_OBJS)
#	$(CC) -static $(ZUTEXERCISE_OBJS) -o $@ -lpthread -lcrypto

