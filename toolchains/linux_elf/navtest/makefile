
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	ppm_installed \
	navtest-pie \

NT_SRCS=\
	NavTest.cpp \
	UIMgr.cpp \
	UIPane.cpp \

#	SockMgr.cpp \

########## Include more modules
include ../make.modules

CRYPTO_MODULE_OBJS += time_posix.o 

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
COMMON_UTILS_OBJS=\
	LiteLib.o \
	standard_malloc_factory.o \
	ambient_malloc.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	hash_table.o \
	ZRectangle.o \
	TabIconPainter.o \
	hash_to_hex.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	xax_extensions.o \
	AppIdentity.o \
	linked_list.o \
	SyncFactory_Pthreads.o \

NAVIGATION_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/navigation_module
NAVIGATION_MODULE_OBJS=\
	NavigationProtocol.o \
	NavigationBlob.o \
	NavMgr.o \
	NavSocket.o \
	NavInstance.o \
	AckWait.o \

MODULES=$(CRYPTO_MODULE) $(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE) $(NAVIGATION_MODULE)
MODULE_OBJS=$(CRYPTO_MODULE_OBJS) $(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS) $(NAVIGATION_MODULE_OBJS)

NT_OBJS=$(call mapSrcsToObjs, $(NT_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(NT_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \

#	`pkg-config --cflags gtk+-2.0` \

CFLAGS=\
	-g -pie -fpie \
	-lpng \
	-lpthread \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

#	-lnetpbm \
#	`pkg-config --libs gtk+-2.0` \


########## Include

include ../make.in

########## Rules

$(BUILD)/ppm_installed: /usr/include/libpbm.h
	touch $@

/usr/include/libpbm.h:
	sudo apt-get install libnetpbm10-dev

$(BUILD)/navtest-pie: $(NT_OBJS)
	$(CXX) -pie $(CFLAGS) $(NT_OBJS) -o $@
