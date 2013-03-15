
########## Variables

ZOOG_ROOT=../../..

include ../make.arch

TARGETS=zftp_zoog.signed

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
BOOTBLOCKS_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/bootblocks/
ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module/

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
	linked_list.o \
	malloc_factory.o \
	ambient_malloc.o \
	malloc_factory_operator_new.o \
	zoog_malloc_factory.o \

ELF_COMMON_OBJS=\
	pure_virtual.o \
	string_compat.o \
	xax_skinny_network.o \
	ZClock.o \
	LegacyZClock.o \
	zutex_sync.o \
	packetfilter.o \
	SyncFactory_Zutex.o \
	SyncFactory_TimedZutex.o \
	ThreadFactory_Cheesy.o \
	XStreamXdt.o \
	ZeroCopyBuf_Xnb.o \
	ZeroCopyView.o \
	FragmentReassembly.o \
	XSNReceiveAccounting.o \
	ZStats.o \
	perf_measure.o \

#	hash_table.o \

# elfobj stuff needed by relocate_this
ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_MODULE_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

BOOTBLOCKS_OBJS=\
	elf_start_$(ARCH).o \
	relocate_this.o \
	dummy_main.o \

ZFTP_OBJS=\
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
	pure_virtual.o \
	zlc_util.o \
	FreshnessSeal.o \
	SocketFactory_Skinny.o \
	SocketFactory.o \
	tree_math.o \
	ZFileReplyFromServer.o \
	ZFileClient.o \
	ZCachedName.o \
	ErrorFileResult.o \
	InternalFileResult.o \
	ValidFileResult.o \
	ZFileServer.o \
	ZLookupServer.o \
	ZLCEmitXdt.o \
	ZWireFileRequest.o \
	ZLCTestHarness.o \
	ZFetch.o \
	reference_file.o \
	ZFastFetch.o \
	SendBufferFactory_Xnb.o \
	DataRange.o \
	OutboundRequest.o \
	RequestBuilder.o \
	ValidatedZCB.o \
	MemBuf.o \

########## Include more modules

include ../make.modules

##########


MODULES= $(COMMON_UTILS_MODULE) $(BOOTBLOCKS_MODULE) $(ELFOBJ_MODULE) $(ELF_COMMON_MODULE) $(ZFTP_MODULE) $(CRYPTO_MODULE) $(KEYVAL_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(BOOTBLOCKS_OBJS) $(ELFOBJ_MODULE_OBJS) $(ELF_COMMON_OBJS) $(ZFTP_OBJS) $(CRYPTO_MODULE_OBJS) $(KEYVAL_MODULE_OBJS)

ELF_FLATTEN_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/elf_flatten
LIBGCC_PARTS_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/libgcc_parts
PREREQUISITE_DIRS=$(ELF_FLATTEN_DIR) $(LIBGCC_PARTS_DIR)
ELF_FLATTEN_BINARY=$(ELF_FLATTEN_DIR)/build/elf_flatten

ZBC_SRCS=\
	zftp_zoog.c \
	zbig_main.cpp \
	bulk_test.cpp \
	ZBCContext.c \

DEP_SRCS=$(ZBC_SRCS)

# NB elf_start.o must appear first, so that its function definition is
# the first executable byte in the file, so that when we strip off the
# Elf header, we have a program with an entry point at file offset 0.
ZBC_OBJS=$(call mapSrcsToObjs,$(ZBC_SRCS) $(MODULE_OBJS)) \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/_udivdi3.o

DEFINE_ENTRY_POINT=-Xlinker --entry -Xlinker _elfstart

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/keyval_module \

CFLAGS=\
	-g -pie -fpie -static -nostdlib \
	$(DEFINE_ENTRY_POINT) \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-D_GNU_SOURCE \
	-DZFTP_USE_OPENSSL \
	-DZLC_USE_PRINTF \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DZOOG_NO_STANDARD_INCLUDES \

# _GNU_SOURCE needed to get stat64

########## Include

include ../make.in

########## Rules

$(BUILD)/elf_start_$(ARCH).o: elf_start_$(ARCH).S
	$(CC) -fpic -c $? -o $@

$(BUILD)/zftp_zoog.elf: $(ZBC_OBJS)
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
		--skeypair $(CRYPTO_DIR)/keys/zftp-zoog.keys \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc ` \
