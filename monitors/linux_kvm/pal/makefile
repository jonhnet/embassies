########## Variables

# When we get around to supporting another ARCH, we'll want to
# hoist this definition into an include file.
ARCH=i386

ZOOG_ROOT=../../..

TARGETS=zoog_kvm_pal

PREREQUISITE_DIRS=

include $(ZOOG_ROOT)/toolchains/linux_elf/make.modules

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_MODULE_OBJS=\
	LiteLib.o \
	zoog_malloc_factory.o \
	ambient_malloc.o \
	cheesy_malloc_factory.o \
	malloc_factory.o \
	malloc_factory_operator_new.o \
	concrete_mmap_xdt.o \
	cheesymalloc.o \
	cheesylock.o \
	lite_log2.o \

#CRYPTO_MODULE=$(ZOOG_ROOT)/common/crypto
#CRYPTO_MODULE_OBJS=\
#	bignum.o \
#	rijndael.o \
#	rsa.o \
#	sha2big.o \
#	sha2small.o \
#	vmac.o \
#	CryptoException.o \
#	DomainName.o \
#	RandomSupply.o \
#	ZBinaryRecord.o \
#	ZCert.o \
#	ZCertChain.o \
#	ZDelegationRecord.o \
#	ZKey.o \
#	ZKeyLinkRecord.o \
#	ZKeyPair.o \
#	ZPrivKey.o \
#	ZPubKey.o \
#	ZPubKeyRecord.o \
#	ZRecord.o \
#	ZSigRecord.o \
#	crypto.o \
#	ByteStream.o \

LKVM_COMMON_MODULE=$(ZOOG_ROOT)/monitors/linux_kvm/common
LKVM_COMMON_MODULE_OBJS=\
	dummy_main.o \
	getCurrentTime.o \

MODULES=$(COMMON_UTILS_MODULE) $(LKVM_COMMON_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_MODULE_OBJS) $(LKVM_COMMON_MODULE_OBJS) $(CRYPTO_MODULE_OBJS)

PAL_SRCS=\
	zoog_kvm_pal.cpp \
	KvmPal.cpp \

DEP_SRCS=$(PAL_SRCS)

PAL_OBJS=$(call mapSrcsToObjs, $(PAL_SRCS) $(MODULE_OBJS))

DEFINE_ENTRY_POINT=-Xlinker --entry -Xlinker kvm_start

INCS=\
	-I$(ZOOG_ROOT)/monitors/linux_kvm/common \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \

LIBS=\

CFLAGS=\
	-pie -fpie -static \
	-g \
	$(DEFINE_ENTRY_POINT) \
	$(INCS) \
	$(LIBS) \
	-DDISCOVERY_SERVER_PATH='"$(realpath $(ZOOG_ROOT)/monitors/linux_dbg/discovery/build/discovery_server)"' \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

#	-DDEBUG_ZUTEX_OWNER=1 \

ADJ_LINKER_SCRIPT=\
	$(ZOOG_ROOT)/toolchains/linux_elf/common/adjust_linker_script.py

########## Include

include ../make.in

########## Rules

$(BUILD)/linker_script: $(ADJ_LINKER_SCRIPT)
	$(ADJ_LINKER_SCRIPT) 0x10000 > $@

$(BUILD)/zoog_kvm_pal: $(PAL_OBJS) $(BUILD)/linker_script
	$(CXX) $(CFLAGS) $(PAL_OBJS) -T$(BUILD)/linker_script -o $@ -lcrypto
