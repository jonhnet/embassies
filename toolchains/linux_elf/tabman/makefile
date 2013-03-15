
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	tabman-pie \

TM_SRCS=\
	TabMan.cpp \
	Page.cpp \
	ZoogBox.cpp \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils/
COMMON_UTILS_OBJS=\
	LiteLib.o \
	standard_malloc_factory.o \
	linked_list.o \
	xax_network_utils.o \
	cheesy_snprintf.o \
	hash_table.o \
	AppIdentity.o \
	ambient_malloc.o \

LINUX_ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUX_ELF_COMMON_OBJS=\
	xax_extensions.o \
	XvncControlProtocol.o \

NAVIGATION_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/navigation_module
NAVIGATION_MODULE_OBJS=\
	NavigationProtocol.o \
	NavigationBlob.o \
	NavSocket.o \

########## Include more modules
include ../make.modules

CRYPTO_MODULE_OBJS += time_posix.o

MODULES=$(COMMON_UTILS_MODULE) $(NAVIGATION_MODULE) $(LINUX_ELF_COMMON_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(NAVIGATION_MODULE_OBJS) $(LINUX_ELF_COMMON_OBJS) $(CRYPTO_MODULE_OBJS)

TM_OBJS=$(call mapSrcsToObjs, $(TM_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(TM_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	`pkg-config --cflags gtk+-2.0` \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	`pkg-config --libs gtk+-2.0` \


########## Include

include ../make.in

########## Rules

$(BUILD)/tabman-pie: $(TM_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

