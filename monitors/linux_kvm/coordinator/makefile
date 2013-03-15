########## Variables

# When we get around to supporting another ARCH, we'll want to
# hoist this definition into an include file.
ARCH=i386

ZOOG_ROOT=../../..

TARGETS=zoog_kvm_coordinator

PREREQUISITE_DIRS=

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
	pure_virtual.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	format_ip.o \
	Table.o \
	SockaddrHashable.o \
	ambient_malloc.o \
	ZRectangle.o \
	GrowBuffer.o \
	TunIDAllocator.o \

LINUXELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUXELF_COMMON_MODULE_OBJS=\
	SyncFactory_Pthreads.o \
	ThreadFactory_Pthreads.o \

ZOOG_KVM_COMMON_MODULE=$(ZOOG_ROOT)/monitors/linux_kvm/common
ZOOG_KVM_COMMON_MODULE_OBJS=\
	safety_check.o \
	LongMessageAllocator.o \
	ShmAllocator.o \
	MmapAllocator.o \

MONITORS_COMMON_MODULE=$(ZOOG_ROOT)/monitors/common
MONITORS_COMMON_MODULE_OBJS=\
	BlitterViewport.o \
	BlitterCanvas.o \
	BlitterManager.o \
	ViewportHandle.o \
	DeedEntry.o \

XBLIT_MODULE=$(ZOOG_ROOT)/monitors/linux_common/xblit
XBLIT_MODULE_OBJS=\
	Xblit.o \
	XblitCanvas.o \
	LabelWindow.o \
	BlitterViewport.o \
	BlitterCanvas.o \
	BlitterManager.o \
	UIEventQueue.o \
	XWork.o \

SHM_MODULE=$(ZOOG_ROOT)/monitors/linux_common/shm
SHM_MODULE_OBJS=\
	Shm.o \

MODULES=$(COMMON_UTILS_MODULE) \
	$(LINUXELF_COMMON_MODULE) \
	$(ZOOG_KVM_COMMON_MODULE) \
	$(XBLIT_MODULE) \
	$(SHM_MODULE) \
	$(CRYPTO_MODULE) \
	$(MONITORS_COMMON_MODULE) \
	$(MONITOR_CRYPTO_MODULE) \

MODULE_OBJS=$(COMMON_UTILS_MODULE_OBJS) \
	$(LINUXELF_COMMON_MODULE_OBJS) \
	$(ZOOG_KVM_COMMON_MODULE_OBJS) \
	$(XBLIT_MODULE_OBJS) \
	$(SHM_MODULE_OBJS) \
	$(CRYPTO_MODULE_OBJS) \
	$(MONITORS_COMMON_MODULE_OBJS) \
	$(MONITOR_CRYPTO_MODULE_OBJS) \


SRCS=\
	App.cpp \
	Listener.cpp \
	Message.cpp \
	NetIfc.cpp \
	Router.cpp \
	ForwardingEntry.cpp \
	zoog_kvm_coordinator.cpp \
	AppIfc.cpp \
	Packet.cpp \
	TunnelPacket.cpp \
	MonitorPacket.cpp \
	AppLink.cpp \
	UnicastIfc.cpp \
	BroadcastIfc.cpp \
	TunnelIfc.cpp \
	CanvasIDHashable.cpp \
	getCurrentTime.cpp \
	LinuxKvmCanvasAcceptor.cpp \
	LongPacket.cpp \

#	CoordinatorCanvas.cpp \

DEP_SRCS=$(SRCS)

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/monitors/linux_kvm/common \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(BUILD) \

LIBS=\
	-lpthread\
	-lcap \
	-lX11 -lXext \

ROOT_KEYS=$(ZOOG_ROOT)/toolchains/linux_elf/crypto/keys/root.keys

CFLAGS=\
	-g \
	$(INCS) \
	$(LIBS) \
	-DDISCOVERY_SERVER_PATH='"$(realpath $(ZOOG_ROOT)/monitors/linux_dbg/discovery/build/discovery_server)"' \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-DDEBUG_VULNERABLY=1 \
	-DDEBUG_EVERY_PACKET=0 \

#	-DDEBUG_ZUTEX_OWNER=1 \

########## Include

include ../make.in

########## Rules

$(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c: $(ROOT_KEYS)
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ zoog_root_key_bytes $(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c

$(BUILD)/zoog_root_key.o: $(BUILD)/zoog_root_key.c

# Add a dependency to the auto-generated header file
$(BUILD)/MonitorCrypto.o: $(BUILD)/zoog_root_key.h 

SPECIAL_DEBUG_LINKAGE=-Wl,-R -Wl,/home/jonh/zoog/toolchains/linux_elf/lib_links/dbg_coord_hang
$(BUILD)/zoog_kvm_coordinator: $(OBJS) $(BUILD)/zoog_root_key.o
	$(CXX) $^ -o $@ $(SPECIAL_DEBUG_LINKAGE)
	sudo setcap Cap_net_raw,cap_net_admin=ep $@
