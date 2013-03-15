########## Variables

# When we get around to supporting another ARCH, we'll want to
# hoist this definition into an include file.
ZOOG_ROOT=../../..

include $(ZOOG_ROOT)/toolchains/linux_elf/make.arch

TARGETS=zoog_kvm_monitor

PREREQUISITE_DIRS=

########## Include more modules

include $(ZOOG_ROOT)/toolchains/linux_elf/make.modules


COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_MODULE_OBJS=\
	malloc_factory.o \
	standard_malloc_factory.o \
	LiteLib.o \
	malloc_factory_operator_new.o \
	linked_list.o \
	hash_table.o \
	CoalescingAllocator.o \
	SyncFactory_Pthreads.o \
	pure_virtual.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	corefile.o \
	ambient_malloc.o \
	ZRectangle.o \
	GrowBuffer.o \
	TunIDAllocator.o \

ELFOBJ_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/elfobj
ELFOBJ_MODULE_OBJS=\
	elf_flat_reader.o \
	elfobj.o \

LINUX_ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUX_ELF_COMMON_MODULE_OBJS=\

ZOOG_KVM_COMMON_MODULE=$(ZOOG_ROOT)/monitors/linux_kvm/common
ZOOG_KVM_COMMON_MODULE_OBJS=\
	safety_check.o \
	LongMessageAllocator.o \
	ShmAllocator.o \
	MmapAllocator.o \

XBLIT_MODULE=$(ZOOG_ROOT)/monitors/linux_common/xblit
XBLIT_MODULE_OBJS=\
	UIEventQueue.o \

SHM_MODULE=$(ZOOG_ROOT)/monitors/linux_common/shm
SHM_MODULE_OBJS=\
	Shm.o \

MONITORS_COMMON_MODULE=$(ZOOG_ROOT)/monitors/common
MONITORS_COMMON_MODULE_OBJS=\
	BlitterViewport.o \
	BlitterCanvas.o \
	BlitterManager.o \
	ViewportHandle.o \
	DeedEntry.o \

MODULES=$(COMMON_UTILS_MODULE) \
	$(CRYPTO_MODULE) \
	$(ELFOBJ_MODULE) \
	$(ZOOG_KVM_COMMON_MODULE) \
	$(LINUX_ELF_COMMON_MODULE) \
	$(XBLIT_MODULE) \
	$(SHM_MODULE) \
	$(MONITORS_COMMON_MODULE) \
	$(MONITOR_CRYPTO_MODULE) \

MODULE_OBJS=$(COMMON_UTILS_MODULE_OBJS) \
	$(CRYPTO_MODULE_OBJS) \
	$(ELFOBJ_MODULE_OBJS) \
	$(ZOOG_KVM_COMMON_MODULE_OBJS) \
	$(LINUX_ELF_COMMON_MODULE_OBJS) \
	$(XBLIT_MODULE_OBJS) \
	$(SHM_MODULE_OBJS) \
	$(MONITORS_COMMON_MODULE_OBJS) \
	$(MONITOR_CRYPTO_MODULE_OBJS) \

VM_SRCS=\
	ZoogVM.cpp \
	ZoogVCPU.cpp \
	MemSlot.cpp \
	SmallIntegerAllocator.cpp \
	NetBuffer.cpp \
	NetBufferTable.cpp \
	CoordinatorConnection.cpp \
	Packet.cpp \
	safety_check.cpp \
	MonitorArgs.cpp \
	ZutexQueue.cpp \
	ZutexWaiter.cpp \
	ZutexTable.cpp \
	AlarmThread.cpp \
	MmapOverride.cpp \
	HostAlarms.cpp \
	CanvasAcceptor.cpp \
	VCPUPool.cpp \
	RPCToken.cpp \
	CallCounts.cpp \

#	ViewportDelegator.cpp \

MONITOR_SRCS=\
	$(VM_SRCS) \
	zoog_kvm_monitor.cpp \

DEP_SRCS=$(VM_SRCS) $(MONITOR_SRCS)

MONITOR_OBJS=$(call mapSrcsToObjs, $(MONITOR_SRCS) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(BUILD) \

LIBS=\
	-lpthread\
	-ldl\

CFLAGS=\
	-g \
	$(INCS) \
	$(LIBS) \
	-DDISCOVERY_SERVER_PATH='"$(realpath $(ZOOG_ROOT)/monitors/linux_dbg/discovery/build/discovery_server)"' \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DELFOBJ_USE_LIBC=1 \
	-DDEBUG_VULNERABLY=1 \
	-DZLC_USE_PRINTF=1 \
	-DUSE_FILE_IO=1 \
	-DUSE_PHTREADS=1 \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \

#	-DDEBUG_ZUTEX_OWNER=1 \
#	-DDBG_SEND_FAILURE=1 \

ROOT_KEYS=$(ZOOG_ROOT)/toolchains/linux_elf/crypto/keys/root.keys

########## Include

include ../make.in

########## Rules

$(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c: $(ROOT_KEYS)
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ zoog_root_key_bytes $(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c

$(BUILD)/zoog_root_key.o: $(BUILD)/zoog_root_key.c

# Add a dependency to the auto-generated header file
$(BUILD)/MonitorCrypto.o: $(BUILD)/zoog_root_key.h 


#$(BUILD)/mmap_override_test: mmap_override_test.o MmapOverride.o
#	$(CC) $^ -o $@

$(BUILD)/local_elf.h: /usr/include/elf.h
	grep -v '#include <features.h>' < $< > $@

$(BUILD)/zoog_kvm_monitor: $(MONITOR_OBJS) $(BUILD)/zoog_root_key.o
	$(CXX) $^ -o $@

$(BUILD)/corefile.o $(BUILD)/ZoogVM.o $(BUILD)/ZoogVCPU.o: $(BUILD)/local_elf.h
