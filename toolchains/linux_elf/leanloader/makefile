
########## Variables

ZOOG_ROOT=../../..

include ../make.arch

TARGETS=leanloader.signed

BOOTBLOCKS_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/bootblocks/
BOOTBLOCKS_OBJS=\
	elf_start_$(ARCH).o \
	relocate_this.o \
	dummy_main.o \

## elfobj stuff needed by relocate_this
ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_MODULE_OBJS=\
	elfobj.o \
	elf_flat_reader.o \
	perf_measure.o \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	LiteLib.o \
	ambient_malloc.o \
	zoog_malloc_factory.o \
	cheesy_malloc_factory.o \
	concrete_mmap_xdt.o \
	cheesymalloc.o \
	cheesylock.o \
	malloc_factory_operator_new.o \
	lite_log2.o \
	ZRectangle.o \
	cheesy_snprintf.o \

#	cheesy_thread.o \
#	cheesy_malloc_factory.o \
#	xax_util.o \
#	simple_network.o \
#	xax_network_utils.o \
#	hash_table.o \
#	format_ip.o \
#	linked_list.o \
#	malloc_factory.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	get_current_time_dummy.o \

#	pure_virtual.o \
#	string_compat.o \
#	xax_skinny_network.o \
#	ZClock.o \
#	LegacyZClock.o \
#	zutex_sync.o \
#	packetfilter.o \
#	SyncFactory_Zutex.o \
#	SyncFactory_TimedZutex.o \
#	ThreadFactory_Cheesy.o \
#	XStreamXdt.o \
#	ZeroCopyBuf_Xnb.o \
#	ZeroCopyView.o \
#	FragmentReassembly.o \
#	XSNReceiveAccounting.o \
#	ZStats.o \
#	perf_measure.o \
#
##	hash_table.o \
#

#ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module/
#ZFTP_OBJS=\
#	ZCache.o \
#	ZOrigin.o \
#	malloc_factory_operator_new.o \
#	ZLookupClient.o \
#	ZLookupRequest.o \
#	ZSyntheticFileRequest.o \
#	ZFileRequest.o \
#	ZCachedFile.o \
#	ValidatedMerkleRecord.o \
#	ZLCArgs.o \
#	ZBlockCache.o \
#	ZBlockCacheRecord.o \
#	ZNameRecord.o \
#	pure_virtual.o \
#	zlc_util.o \
#	FreshnessSeal.o \
#	SocketFactory_Skinny.o \
#	SocketFactory.o \
#	tree_math.o \
#	ZFileReplyFromServer.o \
#	ZFileClient.o \
#	ZCachedName.o \
#	ErrorFileResult.o \
#	InternalFileResult.o \
#	ValidFileResult.o \
#	ZFileServer.o \
#	ZLookupServer.o \
#	ZLCEmitXdt.o \
#	ZWireFileRequest.o \
#	ZLCTestHarness.o \
#	ZFetch.o \
#	reference_file.o \
#	ZFastFetch.o \
#	SendBufferFactory_Xnb.o \
#	DataRange.o \
#	OutboundRequest.o \
#	RequestBuilder.o \

########## Include more modules

include ../make.modules

##########


MODULES=\
	$(BOOTBLOCKS_MODULE) \
	$(COMMON_UTILS_MODULE) \
	$(ELFOBJ_MODULE) \
	$(CRYPTO_MODULE) \
	$(ELF_COMMON_MODULE) \

#	$(ZFTP_MODULE) \
#	$(KEYVAL_MODULE)

MODULE_OBJS=\
	$(BOOTBLOCKS_OBJS) \
	$(COMMON_UTILS_OBJS) \
	$(ELFOBJ_MODULE_OBJS) \
	$(CRYPTO_MODULE_OBJS) \
	$(ELF_COMMON_OBJS) \

#	$(ZFTP_OBJS) \
#	$(KEYVAL_MODULE_OBJS)

ELF_FLATTEN_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/elf_flatten
LIBGCC_PARTS_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/libgcc_parts
PREREQUISITE_DIRS=$(ELF_FLATTEN_DIR) $(LIBGCC_PARTS_DIR)
ELF_FLATTEN_BINARY=$(ELF_FLATTEN_DIR)/build/elf_flatten

LL_SRCS=\
	leanloader.cpp \
	TinyPPM.cpp \

DEP_SRCS=$(LL_SRCS)

# NB elf_start.o must appear first, so that its function definition is
# the first executable byte in the file, so that when we strip off the
# Elf header, we have a program with an entry point at file offset 0.
LL_OBJS=$(call mapSrcsToObjs,$(LL_SRCS) $(MODULE_OBJS)) \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/_udivdi3.o \
	$(BUILD)/vendor-a-chain.o \
	$(BUILD)/splash_image.o \

DEFINE_ENTRY_POINT=-Xlinker --entry -Xlinker _elfstart

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/keyval_module \

CFLAGS=\
	-O2 -g -pie -fpie -static -nostdlib \
	$(DEFINE_ENTRY_POINT) \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-D_GNU_SOURCE \
	-DZFTP_USE_OPENSSL \
	-DZLC_USE_PRINTF \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DZOOG_NO_STANDARD_INCLUDES \

#	-fdata-sections -ffunction-sections \
#	-Xlinker --gc-sections \

# _GNU_SOURCE needed to get stat64

########## Include

include ../make.in

########## Rules

$(BUILD)/vendor-a-chain.h $(BUILD)/vendor-a-chain.c: $(CRYPTO_DIR)/certs/vendor-a.chain
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ vendor_a_chain $(BUILD)/vendor-a-chain.h $(BUILD)/vendor-a-chain.c

$(BUILD)/leanloader.o: $(BUILD)/vendor-a-chain.h

$(BUILD)/splash_image.ppm: splash_image.ppm.gz
	gzip -d < $< > $@

$(BUILD)/splash_image.h $(BUILD)/splash_image.c: $(BUILD)/splash_image.ppm
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ splash_image $(BUILD)/splash_image.h $(BUILD)/splash_image.c

$(BUILD)/elf_start_$(ARCH).o: elf_start_$(ARCH).S
	$(CC) -fpic -c $? -o $@

$(BUILD)/leanloader.o: $(BUILD)/splash_image.h

$(BUILD)/leanloader.elf: $(LL_OBJS)
	g++ $(DEFINE_ENTRY_POINT) -pie -g -static $^ -o $@ -lcrypto

$(BUILD)/%.raw: $(BUILD)/%.elf $(ELF_FLATTEN_BINARY)
	$(ELF_FLATTEN_BINARY) \
		--input-image $< \
		--include-elf-header true \
		--insert-entry-jump true \
		--allocate-new-stack-size 0x20000 \
		--emit-binary $@

$(BUILD)/%.cert: $(BUILD)/%.raw 
	$(CRYPTO_DIR)/$(BUILD)/crypto_util \
    --binary $^ \
		--cert $@ \
		--skeypair $(CRYPTO_DIR)/keys/vendor-a.keys \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc ` \
