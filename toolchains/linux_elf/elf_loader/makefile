
########## Variables

include ../make.arch

ZOOG_ROOT=../../..

TARGETS=\
	elf_loader.vendor_a.signed \
	elf_loader.vendor_b.signed \
	elf_loader.zoog_identity_test.signed \
	elf_loader.app_start_time.signed \
	elf_loader.hello.signed \
	elf_loader.midori.signed \
	elf_loader.midori_hot.signed \
	elf_loader.midori_warm.signed \
	elf_loader.navtest.signed \
	elf_loader.tabman.signed \
	elf_loader.keyval.signed \
	elf_loader.keyval-test.signed \
	elf_loader.getaddr.signed \
	elf_loader.results_search_com.signed \
	elf_loader.widgets_retailer_com.signed \
	elf_loader.spreadsheet_office_org.signed \
	elf_loader.wordprocessor_office_org.signed \
	elf_loader.raster_graphics_net.signed \
	elf_loader.vector_graphics_net.signed \
	elf_loader.gnucash_org.signed \
	elf_loader.marble_maps_net.signed \
	elf_loader.xclock.signed \
	elf_loader.xeyes.signed \
	elf_loader.zhyperoid.signed \
	elf_loader.zhyperoid-log-paths.signed \
	elf_loader.craigslist.signed \
	elf_loader.ebay.signed \
	elf_loader.microsoft.signed \
	elf_loader.reddit.signed \
	elf_loader.craigslist_warm.signed \
	elf_loader.ebay_warm.signed \
	elf_loader.microsoft_warm.signed \
	elf_loader.reddit_warm.signed \
	elf_loader.cpuid_test.signed \

# TODO this makefile is old, and should inherit most of these deps from ../make.modules
# instead of declaring them explicitly here.

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
#ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module
BOOTBLOCKS_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/bootblocks/

COMMON_UTILS_OBJS=\
	LiteLib.o \
	hash_table.o \
	cheesymalloc.o \
	lite_log2.o \
	simple_network.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	format_ip.o \
	SocketFactory.o \
	malloc_trace.o \
	ambient_malloc.o \
	malloc_factory_operator_new.o \
	ZLCEmitXdt.o \

ELF_COMMON_OBJS=\
	ElfArgs.o \
	cheesylock.o \
	cheesy_malloc_factory.o \
	malloc_factory.o \
	zutex_sync.o \
	debug_util.o \
	xoverlayfs.o \
	ZQueue.o \
	elf_flat_reader.o \
	binsearch.o \
	SyncFactory_Zutex.o \
	SyncFactory_TimedZutex.o \
	zoog_malloc_factory.o \
	string_compat.o \
	pure_virtual.o \
	ThreadFactory_Cheesy.o \
	XStreamXdt.o \
	SocketFactory_Skinny.o \
	SocketFactory_Skinny.o \
	ZeroCopyBuf_Xnb.o \
	ZeroCopyView.o \
	ZClock.o \
	LegacyZClock.o \
	FragmentReassembly.o \
	LegacyPF.o \
	XSNReceiveAccounting.o \
	ZStats.o \
	FastFetchHelper.o \

ELFOBJ_OBJS=\
	elfobj.o \


BOOTBLOCKS_OBJS=\
	elf_start_$(ARCH).o \
	relocate_this.o \
	dummy_main.o \

ZFTP_LOCAL_OBJS=\
	ChunkDecoder.o \
	SendBufferFactory_Xnb.o \
	ZFastFetch.o \
	ZCompressionStub.o \

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
#	zlc_util.o \
#	FreshnessSeal.o \
#	tree_math.o \
#	ZFileReplyFromServer.o \
#	ZFileClient.o \
#	ZCachedName.o \
#	ErrorFileResult.o \
#	InternalFileResult.o \
#	ValidFileResult.o \
#	ZLCEmitXdt.o \
#	ZFastFetch.o \
#	DataRange.o \
#	OutboundRequest.o \
#	RequestBuilder.o \
#	MemBuf.o \
#	ValidatedZCB.o \

########## Include more modules

	include ../make.modules

##########

#	zftp_client_lib.o \
#	debug.o \
#	zftp_util.o \

MODULES=\
	$(ELFOBJ_MODULE) \
	$(COMMON_UTILS_MODULE) \
	$(ELF_COMMON_MODULE) \
	$(ZFTP_MODULE) \
	$(BOOTBLOCKS_MODULE) \
	$(KEYVAL_MODULE) \
	$(CRYPTO_MODULE) \

MODULE_OBJS=\
	$(ELFOBJ_OBJS) \
	$(COMMON_UTILS_OBJS) \
	$(ELF_COMMON_OBJS) \
	$(ZFTP_CORE_OBJS) \
	$(ZFTP_LOCAL_OBJS) \
	$(BOOTBLOCKS_OBJS) \
	$(KEYVAL_MODULE_OBJS) \

#	$(CRYPTO_MODULE_OBJS) \

LIBC_REWRITTEN_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/libc-rewritten/
#ZFTP_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/zftp/
XAXINTERPOSE_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/xaxinterpose
ELF_FLATTEN_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/elf_flatten
#CRYPTO_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/crypto
OPT_CRYPTO_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/opt_crypto
LIBGCC_PARTS_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/libgcc_parts
LINUX_CRYPTO_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/crypto
PREREQUISITE_DIRS=$(LIBC_REWRITTEN_DIR) $(ZFTP_DIR) $(XAXINTERPOSE_DIR) $(ELF_FLATTEN_DIR) $(LIBGCC_PARTS_DIR) $(OPT_CRYPTO_DIR) $(LINUX_CRYPTO_DIR)
ELF_FLATTEN_BINARY=$(ELF_FLATTEN_DIR)/build/elf_flatten

SRCS=\
	ElfArgs.c \
	ElfLoad.c \
	elf_loader.c \
	xinterpose.c \
	handle_table.c \
	xax_posix_emulation.c \
	xax_stack_table.c \
	malloc_trace.c \
	xax_util.c \
	xmaskfs.c \
	xerrfs.c \
	cheesy_thread.c \
	simpipe.c \
	concrete_mmap_xdt.c \
	strace.c \
	KeyValFS.cpp \
	XRamFS.cpp \
	XRDirNode.cpp \
	XRFileNode.cpp \
	fake_ids.c \
	xprocfakeryfs.c \
	xax_skinny_network.c \
	packetfilter.cpp \
	thread_table.c \
	xax_unixdomainsockets.c \
	xi_network.c \
	xbrk.cpp \
	xrandomfs.c \
	stomp_detector.c \
	ordered_collection.c \
	lru_cache.c \
	linked_list.c \
	XaxVFS.cpp \
	ZLCVFS.cpp \
	FilterVFS.c \
	VFSElfReader.cpp \
	PatchPogoFilter.cpp \
	ZFTPDecoder.cpp \
	ZarfileVFS.cpp \
	zlcvfs_init.cpp \
	UnionVFS.cpp \
	XVFSHandleWrapper.cpp \
	XVFSNamespace.cpp \
	DecomposePath.cpp \
	ZutexHandle.cpp \
	perf_measure.cpp \
	exception_test.cpp \
	pthreads_dummies.cpp \
	TraceCollector.cpp \

#	discovery_client.c \

DEP_SRCS=$(SRCS)

SECOND_STAGE_LOADER_IMAGE=\
	$(LIBC_REWRITTEN_DIR)/build/ld.sw.so

# NB elf_start.o must appear first, so that its function definition is
# the first executable byte in the file, so that when we strip off the
# Elf header, we have a program with an entry point at file offset 0.
OBJS=$(call mapSrcsToObjs,elf_start_$(ARCH).o $(SRCS) $(MODULE_OBJS)) $(BUILD)/loader.o \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/_udivdi3.o \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/memcpy.o \
	$(LIBGCC_PARTS_DIR)/$(BUILD)/memcmp.o \

DEFINE_ENTRY_POINT=-Xlinker --entry -Xlinker _elfstart

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/keyval_module \
	-I$(ELF_FLATTEN_DIR) \

NOSTDLIB=-nostdlib -fno-exceptions

CFLAGS=\
	-g -pie -fpie -static $(NOSTDLIB) \
	$(DEFINE_ENTRY_POINT) \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-D_GNU_SOURCE \
	-DNOEXCEPTIONS \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

# _GNU_SOURCE needed to get stat64

########## Include

include ../make.in

########## Rules

.SECONDARY:

$(BUILD)/elf_loader.elf: $(OBJS) $(XAXINTERPOSE_DIR)/build/libxaxinterpose.a $(OPT_CRYPTO_DIR)/build/libopt_crypto.a
	$(CXX) $(DEFINE_ENTRY_POINT) -pie -g -static $(NOSTDLIB) $^ -o $@ \
		-L$(OPT_CRYPTO_DIR)/build \
		-lopt_crypto \
		-lcrypto \

$(BUILD)/loader.o: $(SECOND_STAGE_LOADER_IMAGE) $(ELF_FLATTEN_BINARY)
	$(ELF_FLATTEN_BINARY) \
		--input-image $(SECOND_STAGE_LOADER_IMAGE) \
		--include-elf-header true \
		--emit-binary $(BUILD)/loader.debugbin \
		--emit-c-dir $(BUILD) \
		--emit-c-symbol loader
	$(CC) -c $(BUILD)/loader.c -o $@

$(BUILD)/%.concrete: %.config zguest.config
	m4 -DZOOG_ROOT_VALUE=$(realpath $(ZOOG_ROOT)) < $< > $@

#$(BUILD)/%.config:
#	echo 'include(zguest.config)ENV VENDOR_ID=$*' > $@

#	echo 'include(zguest.config)ENV VENDOR_ID='`echo $@ | sed 's#.*/##' | sed 's#.config##'` > $@

# sign a cert according to the vendor ID in a config file
extractVendor=grep 'ENV VENDOR_ID' $(1)|awk -F= '{print $$2}'

$(BUILD)/elf_loader.%.cert: $(BUILD)/elf_loader.%.raw 
	$(CRYPTO_DIR)/$(BUILD)/crypto_util \
    --binary $^ \
		--cert $@ \
		--skeypair $(CRYPTO_DIR)/keys/$(shell $(call extractVendor,$*.config)).keys \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc ` \

#		--skeypair $(CRYPTO_DIR)/keys/$*.keys \

$(BUILD)/elf_loader.%.raw: $(BUILD)/elf_loader.elf $(BUILD)/%.concrete
	$(ELF_FLATTEN_BINARY) \
		--input-image $< \
		--insert-manifest $(BUILD)/$*.concrete \
		--include-elf-header true \
		--insert-entry-jump true \
		--allocate-new-stack-size 0x20000 \
		--emit-binary $@

$(BUILD)/elf_start_$(ARCH).o: elf_start_$(ARCH).S
	$(CC) -fpic -c $? -o $@
