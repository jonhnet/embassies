########## Variables

ZOOG_ROOT=../../..

include ../make.arch
include ../make.modules

# jonh deprecates apps/old-libpng. We used it for a perf test, to demonstrate the cost
# of deploying a fix to a library with a security hole. But, because this library has
# a security hole, it's getting harder and harder to find on the Internet, remarkably.
# OLD_LIBPNG_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/apps/old-libpng/
# PREREQUISITE_DIRS=$(OLD_LIBPNG_DIR)

TARGETS=\
	zftp_create_zarfile \
	zarfile_tool \
	diff_tool \
	midori.tweaked-zarfile \
	midori.zarfile \
	xclock.zarfile \
	xeyes.zarfile \
	gnumeric.zarfile \
	inkscape.zarfile \
	gnucash.zarfile \
	marble.zarfile \
	zhyperoid.zarfile \
	midori_reddit.zarfile \
	midori_reddit.tweaked-zarfile \
	midori_ebay.zarfile \
	midori_ebay.tweaked-zarfile \
	midori_microsoft.zarfile \
	midori_microsoft.tweaked-zarfile \
	midori_craigslist.zarfile \
	midori_craigslist.tweaked-zarfile \

DEP_SRCS=$(wildcard *.c *.cpp)

CZ_SRCS=\
	zftp_create_zarfile.cpp \
	copy_file.cpp \
	zcz_args.cpp \
	Catalog.cpp \
	CatalogEntry.cpp \
	ZHdr.cpp \
	PHdr.cpp \
	StringTable.cpp \
	ChunkEntry.cpp \
	ChunkTable.cpp \
	Placer.cpp \
	Padding.cpp \
	MmapDecoder.cpp \
	CatalogElfReaderWrapper.cpp \
	ZFSElfReader.cpp \
	TraceDecoder.cpp \
	GreedyPlacement.cpp \
	StablePlacement.cpp \
	HashWriter.cpp \
	StdioWriter.cpp \
	FileScanner.cpp \
	ChunkEntry.cpp \
	PosixChunkDecoder.cpp \

ZT_SRCS=\
	zarfile_tool.cpp \
	ZTArgs.cpp \
	PosixChunkDecoder.cpp \
	ChunkEntry.cpp \
	StdioWriter.cpp \
	FileScanner.cpp \

DT_SRCS=\
	diff_tool.cpp \
	DTArgs.cpp \
	ChunkEntry.cpp \
	StdioWriter.cpp \
	Blaminator.cpp \
	FileScanner.cpp \
	PosixChunkDecoder.cpp \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	LiteLib.o \
	cheesy_snprintf.o \
	Set.o \
	hash_table.o \
	Hashable.o \
	malloc_factory.o \
	CoalescingAllocator.o \
	RangeUnion.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj/
ELFOBJ_OBJS=\
	elfobj.o \
	elf_flat_reader.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	debug.o \
	debug_util.o \
	standard_malloc_factory.o \
	linked_list.o \
	SyncFactory_Pthreads.o \

ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module/
ZFTP_OBJS=\
	zftp_util.o \
	reference_file.o \
	ZFSReader.o \
	ZFSRDir.o \
	ZFSRDir_Posix.o \
	ZFSRDir_Union.o \
	ZFSRStat.o \
	ZFSRFile.o \
	ZFSRInode.o \
	ZFSReference.o \
	HashableHash.o \
	FileSystemView.o \
	HandyDirRec.o \
	ChunkDecoder.o \
	DataRange.o \
	ChunkBySize.o \
	zlc_util.o \

# subset the crypto module
CRYPTO_MODULE_OBJS=\
  rijndael.o \
  sha1.o \
  sha2big.o \
  sha2small.o \
  vmac.o \
  crypto.o \
  sha-openssl.o \
  sha-openssl-asm.o \
  sha-openssl-cpuid.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE) $(ZFTP_MODULE) $(ELFOBJ_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS) $(ZFTP_OBJS) $(ELFOBJ_OBJS) $(CRYPTO_MODULE_OBJS)

CZ_OBJS=$(call mapSrcsToObjs, $(CZ_SRCS) $(MODULE_OBJS))
ZT_OBJS=$(call mapSrcsToObjs, $(ZT_SRCS) $(MODULE_OBJS))
DT_OBJS=$(call mapSrcsToObjs, $(DT_SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \

CFLAGS=\
	-g \
	$(INCS) \
	-DSTANDALONE=1 \
	-DZFTP_USE_OPENSSL=1 \
	-DELFOBJ_USE_LIBC=1 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-DZLC_USE_PRINTF=1 \

LIBFLAGS=\
	-lssl -lpthread

HOSTNAME=$(shell hostname)

########## Include

include ../make.in

# Argh. Can't get this to work (having make track changed input files
# for the zarfiles). I hate make.
#ZARFILE_DEPS=$(patsubst %,$(BUILD)/%.dep,$(filter %.zarfile,$(TARGETS)))
#-include $(ZARFILE_DEPS)

########## Rules

$(BUILD)/zftp_create_zarfile: $(CZ_OBJS)
	$(CXX) $(LIBFLAGS) $^ -o $@

# cause empty manifests to spring into existence for those who haven't
# set zarfiles up yet.
%.paths.$(HOSTNAME):
	touch $@

RESOLV_CONF_OVERLAY=--overlay resolv.conf.overlay
ICON_OVERLAY=--overlay ../apps/midori/icon-prune/build/overlay

$(BUILD)/%.zarfile: $(BUILD)/zftp_create_zarfile %.paths.$(HOSTNAME)
	$< --log $(filter-out %zftp_create_zarfile,$^) --zar $@ --include-elf-syms true --dep-file $@.dep $(ICON_OVERLAY) $(RESOLV_CONF_OVERLAY)

#OLD_LIBPNG_OVERLAY=--overlay $(OLD_LIBPNG_DIR)/build/overlay
#$(BUILD)/%.tweaked-zarfile: $(BUILD)/zftp_create_zarfile %.paths.$(HOSTNAME)
#	$< --log $(filter-out %zftp_create_zarfile,$^) --zar $@ --include-elf-syms true --dep-file $@.dep $(OLD_LIBPNG_OVERLAY) $(ICON_OVERLAY) $(RESOLV_CONF_OVERLAY)

$(BUILD)/zarfile_tool: $(ZT_OBJS)
	$(CXX) $(LIBFLAGS) $^ -o $@

$(BUILD)/diff_tool: $(DT_OBJS)
	$(CXX) $(LIBFLAGS) $^ -o $@

