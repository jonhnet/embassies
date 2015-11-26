
########## Variables

ZOOG_ROOT=../../..

include ../make.arch

TARGETS=paltest.signed launcher_test.signed launchee_test.signed xsntest.signed memcpy_test.signed

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	zoog_malloc_factory.o \
	get_current_time_dummy.o \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	cheesy_thread.o \
	concrete_mmap_xdt.o \
	cheesymalloc.o \
	LiteLib.o \
	cheesylock.o \
	lite_log2.o \
	cheesy_malloc_factory.o \
	xax_util.o \
	simple_network.o \
	xax_network_utils.o \
	hash_table.o \
	cheesy_snprintf.o \
	format_ip.o \
	perf_measure.o \
	string_compat.o \
	malloc_factory_operator_new.o \
	ambient_malloc.o \
	ZRectangle.o \
	SocketFactory.o \

XSN_COMMON_UTILS_OBJS=\
	SyncFactory_Zutex.o \
	SyncFactory_TimedZutex.o \
	pure_virtual.o \
	malloc_factory_operator_new.o \
	LegacyZClock.o \
	ZeroCopyBuf_Xnb.o \
	ZeroCopyView.o \
	linked_list.o \
	SocketFactory.o \
	SocketFactory_Skinny.o \
	ThreadFactory_Cheesy.o \
	malloc_factory.o \
	XSNReceiveAccounting.o \

#	hash_table.o \

XSN_ELF_COMMON_OBJS=\
	zoog_malloc_factory.o \
	xax_skinny_network.o \
	packetfilter.o \
	zutex_sync.o \
	ZClock.o \
	ZLCEmitXdt.o \
	XStreamXdt.o \
	FragmentReassembly.o \
	string_compat.o \

ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module
ZFTP_MODULE_OBJS=\
	ZCache.o \
	ZOrigin.o \
	malloc_factory_operator_new.o \
	ZLookupClient.o \
	ZLookupRequest.o \
	ZSyntheticFileRequest.o \
	ZFileRequest.o \
	ZCachedFile.o \
	ValidatedMerkleRecord.o \
	ZLCArgs.o \
	ZBlockCache.o \
	ZBlockCacheRecord.o \
	ZNameRecord.o \
	zlc_util.o \
	FreshnessSeal.o \
	tree_math.o \
	ZFileReplyFromServer.o \
	ZFileClient.o \
	ZCachedName.o \
	ErrorFileResult.o \
	InternalFileResult.o \
	ValidFileResult.o \
	ZLCEmitXdt.o \
	SendBufferFactory_Xnb.o \
	ZFastFetch.o \
	ZLCTestHarness.o \
	ZFetch.o \
	reference_file.o \
	DataRange.o \
	OutboundRequest.o \
	RequestBuilder.o \
	ZStats.o \
	MemBuf.o \
	ValidatedZCB.o \
	Buf.o \
	ZFTPRequestPacketInit.o \
	ZCompressionStub.o \

# elfobj stuff needed by relocate_this
ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_MODULE_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

BOOTBLOCKS_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/bootblocks/
BOOTBLOCKS_OBJS=\
	elf_start_$(ARCH).o \
	relocate_this.o \
	dummy_main.o \


########## Include more modules

include ../make.modules

########## 

LIBGCC_PARTS_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/libgcc_parts

MODULES= $(COMMON_UTILS_MODULE) $(ZFTP_MODULE) $(BOOTBLOCKS_MODULE) $(ELF_COMMON_MODULE) $(ELFOBJ_MODULE) $(CRYPTO_MODULE) $(KEYVAL_MODULE)
PAL_MODULE_OBJS=$(COMMON_UTILS_OBJS) $(BOOTBLOCKS_OBJS) $(ELFOBJ_MODULE_OBJS) $(ELF_COMMON_OBJS) $(CRYPTO_MODULE_OBJS) $(KEYVAL_MODULE_OBJS)

ELF_FLATTEN_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/elf_flatten

KEY_SOURCE=$(CRYPTO_MODULE)/build/dev_principal.local
SCRIPTS_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/scripts
CRYPTO_UTIL=$(ZOOG_ROOT)/toolchains/linux_elf/crypto

PREREQUISITE_DIRS=$(ELF_FLATTEN_DIR) $(LIBGCC_PARTS_DIR) $(CRYPTO_UTIL)

ELF_FLATTEN_BINARY=$(ELF_FLATTEN_DIR)/build/elf_flatten

PALTEST_SRCS=\
	paltest.c \
	memtest.c \
	segtest.c \
	nettest.c \
	uitest.c \
	nesttest.cpp \
	vendor_a_pub_key.c \
	alarmtest.c \
	threadtest.c \
	recvtest.c \
	paltest.cert.c \
	VerifyLabelTest.cpp \

XSNTEST_SRCS=\
	xsntest.cpp \
	throughputtest.cpp \
	droptest.cpp \
	EventTimeoutTest.cpp \

LAUNCHEE_TEST_SRCS=\
	relocate_this.c \
	launchee_test.c \

LAUNCHER_TEST_SRCS=\
	relocate_this.c \
	launcher_test.c \

MEMCPY_TEST_SRCS=\
	memcpy_test.c \

DEP_SRCS=$(PALTEST_SRCS) $(LAUNCHEE_TEST_SRCS) $(LAUNCHER_TEST_SRCS) $(XSNTEST_SRCS) $(MEMCPY_TEST_SRCS)

# NB elf_start.o must appear first, so that its function definition is
# the first executable byte in the file, so that when we strip off the
# Elf header, we have a program with an entry point at file offset 0.
XSN_OBJS = $(XSN_COMMON_UTILS_OBJS) $(XSN_ELF_COMMON_OBJS) $(ZFTP_MODULE_OBJS)
XSN_BUILDS = $(call mapSrcsToObjs, $(XSN_OBJS) ) \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/_udivdi3.o

# for deps. This makefile is fairly broken.
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(BOOTBLOCKS_OBJS) $(ELF_COMMON_OBJS) $(XSN_OBJS)


PALTEST_OBJS=$(call mapSrcsToObjs,$(PALTEST_SRCS) $(PAL_MODULE_OBJS))

XSNTEST_OBJS=$(call mapSrcsToObjs,$(XSNTEST_SRCS) $(PAL_MODULE_OBJS)) \
	$(XSN_BUILDS)

LAUNCHEE_TEST_OBJS=$(call mapSrcsToObjs,$(LAUNCHEE_TEST_SRCS) $(PAL_MODULE_OBJS))
LAUNCHER_TEST_OBJS=$(call mapSrcsToObjs,$(LAUNCHER_TEST_SRCS) $(PAL_MODULE_OBJS))

MEMCPY_TEST_OBJS=$(call mapSrcsToObjs,$(MEMCPY_TEST_SRCS) $(PAL_MODULE_OBJS))

DEFINE_ENTRY_POINT=-Xlinker --entry -Xlinker _elfstart

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/keyval_module \

NOSTDLIB=

CFLAGS=\
	-g -pie -fpie -static $(NOSTDLIB) \
	$(DEFINE_ENTRY_POINT) \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-D_GNU_SOURCE \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-DZFTP_USE_OPENSSL \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

# _GNU_SOURCE needed to get stat64

########## Include

include ../make.in

########## Rules

$(BUILD)/elf_start_$(ARCH).o: elf_start_$(ARCH).S
	$(CC) -fpic -c $? -o $@

$(BUILD)/%.elf:
	$(CXX) $(DEFINE_ENTRY_POINT) -pie -g -static $(NOSTDLIB) $^ -o $@ -lcrypto

$(BUILD)/%.raw: $(BUILD)/%.elf $(ELF_FLATTEN_BINARY)
	$(ELF_FLATTEN_BINARY) \
		--input-image $< \
		--include-elf-header true \
		--insert-entry-jump true \
		--allocate-new-stack-size 0x20000 \
		--emit-binary $@

.PRECIOUS: $(BUILD)/%.raw


$(BUILD)/launchee_test.signed.h $(BUILD)/launchee_test.signed.c: $(BUILD)/launchee_test.signed
	../scripts/binary_to_c_symbols.py $^ launchee_test_signed $(BUILD)/launchee_test.signed.h $(BUILD)/launchee_test.signed.c

$(BUILD)/paltest.cert.h $(BUILD)/paltest.cert.c:
	../scripts/binary_to_c_symbols.py ../crypto/certs/vendor-a.cert vendor_a_cert $(BUILD)/paltest.cert.h $(BUILD)/paltest.cert.c

#
# launchee is special-cased to emit C code to inline into launcher
#
$(BUILD)/launchee_test.elf: $(LAUNCHEE_TEST_OBJS)
	$(CXX) $(DEFINE_ENTRY_POINT) -pie -g -static $(NOSTDLIB) $^ -o $@ -lcrypto

#$(BUILD)/launchee_test_raw.h $(BUILD)/launchee_test_raw.c: $(BUILD)/launchee_test.elf
#	$(ELF_FLATTEN_BINARY) \
#		--input-image $< \
#		--include-elf-header true \
#		--insert-entry-jump true \
#		--allocate-new-stack-size 0x20000 \
#		--emit-c-symbol launchee_test_raw \
#		--emit-c-dir $(BUILD)

$(BUILD)/launchee_test_raw.o: $(BUILD)/launchee_test.signed.c

$(BUILD)/paltest.o: $(BUILD)/paltest.cert.c

# add a dep to an auto-generated header in build dir
$(BUILD)/launcher_test.o: $(BUILD)/launchee_test.signed.h

$(BUILD)/paltest.elf: $(PALTEST_OBJS) 

$(BUILD)/xsntest.elf: $(XSNTEST_OBJS) 

SIGNING_KEYS=$(CRYPTO_UTIL)/keys/vendor-a.keys

$(BUILD)/launchee_test.cert: $(BUILD)/launchee_test.raw 
	$(CRYPTO_UTIL)/$(BUILD)/crypto_util \
    --binary $^ \
		--cert $@ \
		--skeypair $(SIGNING_KEYS) \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc ` \

$(BUILD)/launcher_test.elf: $(LAUNCHER_TEST_OBJS) $(BUILD)/launchee_test.signed.o

$(BUILD)/memcpy_test.elf: $(MEMCPY_TEST_OBJS)

VENDOR_KEYS=$(CRYPTO_UTIL)/keys/vendor-a.keys

$(BUILD)/%.cert: $(BUILD)/%.raw 
	$(CRYPTO_UTIL)/$(BUILD)/crypto_util \
    --binary $^ \
		--cert $@ \
		--skeypair $(VENDOR_KEYS) \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc ` \

$(BUILD)/nesttest.o: $(BUILD)/vendor_a_pub_key.c

$(BUILD)/vendor_a_pub_key.c $(BUILD)/vendor_a_pub_key.h:
	$(CRYPTO_UTIL)/$(BUILD)/pair_to_pub $(VENDOR_KEYS) $(patsubst %.c,%.pub,$@)
	$(SCRIPTS_DIR)/binary_to_c_symbols.py \
		$(patsubst %.c,%.pub,$@) \
		vendor_key \
		$(patsubst %.c,%.h,$@) \
		$@
