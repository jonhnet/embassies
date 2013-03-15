########## Variables

# When we get around to supporting another ARCH, we'll want to
# hoist this definition into an include file.
ARCH=i386

ZOOG_ROOT=../../..

TARGETS=xax_port_monitor

include $(ZOOG_ROOT)/toolchains/linux_elf/make.modules

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_MODULE_OBJS=\
	hash_table.o \
	malloc_factory.o \
	standard_malloc_factory.o \
	LiteLib.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	format_ip.o \
	linked_list.o \
	simple_network.o \
	ambient_malloc.o \
	ZRectangle.o \
	GrowBuffer.o \
	TunIDAllocator.o \

LINUXELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUXELF_COMMON_MODULE_OBJS=\
	SyncFactory_Pthreads.o \
	ThreadFactory_Pthreads.o \

MONITORS_COMMON_MODULE=$(ZOOG_ROOT)/monitors/common
MONITORS_COMMON_MODULE_OBJS=\
	BlitterViewport.o \
	BlitterCanvas.o \
	BlitterManager.o \
	DeedEntry.o \
	ViewportHandle.o \

XBLIT_MODULE=$(ZOOG_ROOT)/monitors/linux_common/xblit
XBLIT_MODULE_OBJS=\
	Xblit.o \
	XblitCanvas.o \
	LabelWindow.o \
	UIEventQueue.o \
	XWork.o \

SHM_MODULE=$(ZOOG_ROOT)/monitors/linux_common/shm
SHM_MODULE_OBJS=\
	Shm.o \

MONITOR_LINUX_DBG_COMMON_MODULE=$(ZOOG_ROOT)/monitors/linux_dbg/common

MODULES= \
	$(COMMON_UTILS_MODULE) \
	$(XBLIT_MODULE) \
	$(LINUXELF_COMMON_MODULE) \
	$(CRYPTO_MODULE) \
	$(SHM_MODULE) \
	$(MONITORS_COMMON_MODULE) \
	$(MONITOR_CRYPTO_MODULE) \
	$(MONITOR_LINUX_DBG_COMMON_MODULE) \

MODULE_OBJS=\
	$(COMMON_UTILS_MODULE_OBJS) \
	$(XBLIT_MODULE_OBJS) \
	$(LINUXELF_COMMON_MODULE_OBJS) \
	$(CRYPTO_MODULE_OBJS) \
	$(SHM_MODULE_OBJS) \
	$(MONITORS_COMMON_MODULE_OBJS) \
	$(MONITOR_CRYPTO_MODULE_OBJS) \

SRCS=\
	PacketQueue.cpp \
	Packets.cpp \
	xpbpool.cpp \
	netifc.cpp \
	xax_port_monitor.cpp \
	Transaction.cpp \
	LinuxDbgCanvasAcceptor.cpp \
	Process.cpp \
	Monitor.cpp \
	MThread.cpp \
	zq.cpp \
	forwarding_table.cpp \
	EventSynchronizer.cpp \
	ServiceThreadPool.cpp \

DEP_SRCS=$(SRCS)

OBJS=$(call mapSrcsToObjs,$(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(BUILD) \

LIBS=\
	-lpthread\
	-lX11 -lXext \

#	-ldmalloc \
#	-lX11 -lxcb -lpthread -lXau -ldl -lXdmcp -lXext \

ROOT_KEYS=$(ZOOG_ROOT)/toolchains/linux_elf/crypto/keys/root.keys

CFLAGS=\
	-g \
	$(INCS) \
	$(LIBS) \
	-DDISCOVERY_SERVER_PATH='"$(realpath $(ZOOG_ROOT)/monitors/linux_dbg/discovery/build/discovery_server)"' \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

#	-DDEBUG_ZUTEX=1 \
#	-DDEBUG_ZUTEX_OWNER=1 \

########## Include

include ../make.in

########## Rules

$(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c: $(ROOT_KEYS)
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ zoog_root_key_bytes $(BUILD)/zoog_root_key.h $(BUILD)/zoog_root_key.c

$(BUILD)/zoog_root_key.o: $(BUILD)/zoog_root_key.c

# Add a dependency to the auto-generated header file
$(BUILD)/MonitorCrypto.o: $(BUILD)/zoog_root_key.h 


$(BUILD)/xax_port_monitor_nocap: $(OBJS) $(BUILD)/zoog_root_key.o
	$(CXX) $^ -o $@

/sbin/setcap:
	sudo apt-get install libcap2-bin

$(BUILD)/xax_port_monitor: $(BUILD)/xax_port_monitor_nocap /sbin/setcap 
	cp $< $@
	sudo setcap Cap_net_raw,cap_net_admin=ep $@
