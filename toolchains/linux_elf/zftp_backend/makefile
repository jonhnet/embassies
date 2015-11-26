########## Variables

ZOOG_ROOT=../../..

include ../make.modules

TARGETS=zftp_backend zbackend_unit_test

ZLC_SRCS=\
	zftp_backend.cpp \
	getCurrentTime.cpp \

ZUT_SRCS=\
	zbackend_unit_test.c \
	tree_math.c \
	getCurrentTime.cpp \

DEP_SRCS=$(ZLC_SRCS) $(ZTH_SRCS)

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
	standard_malloc_factory.o \
	debug_util.o \
	SyncFactory_Pthreads.o \
	SocketFactory.o \
	SocketFactory_Posix.o \
	SocketFactory_Counter.o \
	sockaddr_util.o \
	ThreadFactory_Pthreads.o \
	ZeroCopyBuf_Memcpy.o \
	ZeroCopyView.o \

ZFTP_MODULE=$(ZOOG_ROOT)/common/zftp_module/
ZFTP_OBJS=\
	debug.o \
	ZBlockCache.o \
	ZBlockCacheRecord.o \
	ValidatedMerkleRecord.o \
	ZCachedFile.o \
	ZFileRequest.o \
	ZWireFileRequest.o \
	ZFileServer.o \
	ZFileReplyFromServer.o \
	ZFileClient.o \
	ZCache.o \
	tree_math.o \
	ZLCArgs.o \
	ZLookupClient.o \
	ZOriginFile.o \
	ZOrigin.o \
	ZFSOrigin.o \
	ZStatOrigin.o \
	ZFSReader.o \
	ZFSRInode.o \
	ZFSRFile.o \
	ZFSRStat.o \
	ZFSRDir.o \
	ZFSRDir_Posix.o \
	ZFSReference.o \
	ZRFOrigin.o \
	ZAuthoritativeOrigin.o \
	FreshnessSeal.o \
	ZNameDB.o \
	ZFileDB.o \
	ZBlockDB.o \
	ZDB.o \
	ZDBDatum.o \
	ZFTPIndexConfig.o \
	ZDBCursor.o \
	ZLookupRequest.o \
	ZLookupServer.o \
	ZCachedName.o \
	ZNameRecord.o \
	zlc_util.o \
	ZSyntheticFileRequest.o \
	ZFetch.o \
	reference_file.o \
	InternalFileResult.o \
	ErrorFileResult.o \
	ValidFileResult.o \
	DataRange.o \
	ZLCEmitStdio.o \
	SendBufferFactory_Memcpy.o \
	ZLCTestHarness.o \
	ZFTPApp.o \
	RequestBuilder.o \
	OutboundRequest.o \
	FileSystemView.o \
	ZStats.o \
	HandyDirRec.o \
	ZFSRDir_Union.o \
	MemBuf.o \
	ValidatedZCB.o \
	Buf.o \
	ZFTPRequestPacketInit.o \
	ZCompressionImpl.o \
	CompressedZCB.o \
	ZCompressionZlibStream.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE) $(ZFTP_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS) $(ZFTP_OBJS) $(CRYPTO_MODULE_OBJS)

ZLC_OBJS=$(call mapSrcsToObjs,$(ZLC_SRCS) $(MODULE_OBJS))
ZTH_OBJS=$(call mapSrcsToObjs,$(ZTH_SRCS) $(MODULE_OBJS))
ZUT_OBJS=$(call mapSrcsToObjs,$(ZUT_SRCS) $(MODULE_OBJS))


INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZFTP_MODULE) \

CFLAGS=\
	-g \
	$(INCS) \
	-DSTANDALONE=1 \
	-DZLC_USE_PERSISTENT_STORAGE=1 \
	-DZLC_INCLUDE_SERVERS=1 \
	-DZLC_USE_ORIGINS=1 \
	-DZLC_USE_PRINTF=1 \

#	-DDEBUG=1 \

LIBFLAGS=\
	-lpthread \
	-ldb \
	-lz \

########## Include

include ../make.in

########## Rules

$(BUILD)/zftp_backend: $(ZLC_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/zbackend_unit_test: $(ZUT_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

run_backend_server: $(BUILD)/zftp_backend
	$(BUILD)/zftp_backend --origin-filesystem true --origin-reference true --listen-zftp tunid --listen-lookup tunid --index-dir zftp_index_origin
