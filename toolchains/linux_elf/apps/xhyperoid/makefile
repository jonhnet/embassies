
########## Variables

ZOOG_ROOT=../../../..

TARGETS=\
	zhyperoid-pie \

ZH_SRCS=\
	hyperoid.c \
	roidsupp.c \
	sound.c \
	zooggraphics.c \
	convxpm.c \
	zoogcpp.cpp \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_MODULE_OBJS=\
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

LINUX_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUX_COMMON_MODULE_OBJS=\
	getCurrentTime.o \
	perf_measure.o \
	xax_extensions.o \

MODULES=\
	$(CRYPTO_MODULE) \
	$(COMMON_UTILS_MODULE) \
	$(LINUX_COMMON_MODULE) \

MODULE_OBJS=\
	$(CRYPTO_MODULE_OBJS) \
	$(COMMON_UTILS_MODULE_OBJS) \
	$(LINUX_COMMON_MODULE_OBJS) \

ZH_OBJS=$(call mapSrcsToObjs, $(ZH_SRCS) $(MODULE_OBJS)) \
	$(BUILD)/vendor-a-chain.o \

DEP_SRCS=$(ZH_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	-Isrc \
	-I$(BUILD) \
	-I$(BUILD)/xhyperoid-1.2 \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

# this goofy rule gets the source into place, and then recursively invokes
# make so make can find the source files in the VPATH.
pre-all: build/patch-stamp
	make all
	
########## Include

include $(ZOOG_ROOT)/toolchains/linux_elf/make.in
include $(ZOOG_ROOT)/toolchains/linux_elf/make.modules

########## Rules

VPATH+=$(BUILD)/xhyperoid-1.2 src

TARBALL_URL='http://www.svgalib.org/rus/xhyperoid/xhyperoid-1.2.tar.gz'
CACHED_TARBALL=$(ZOOG_ROOT)/external-code/xhyperoid-1.2.tar.gz

$(BUILD)/xhyperoid-1.2.tar.gz:
	make $(BUILD)
	@if [ \! -e $(CACHED_TARBALL) ]; then \
		wget $(TARBALL_URL) -O $(CACHED_TARBALL); \
		echo fetch remote $(CACHED_TARBALL); \
	fi
	cp $(CACHED_TARBALL) $@

$(BUILD)/unpack-stamp: $(BUILD)/xhyperoid-1.2.tar.gz
	tar -C $(BUILD) -xzf $^
	touch $@

$(BUILD)/patch-stamp: $(BUILD)/unpack-stamp
	cat patches/*.patch | (cd build/xhyperoid-1.2/; patch)
	cp build/xhyperoid-1.2/svga.c build/xhyperoid-1.2/zooggraphics.c
	patch build/xhyperoid-1.2/zooggraphics.c < patches/zooggraphics.c.patch-create
	touch $@

$(BUILD)/xhyperiod-1.2/hyperoid.c: $(BUILD)/patch-stamp
	echo $(VPATH)

$(BUILD)/vendor-a-chain.h $(BUILD)/vendor-a-chain.c: $(CRYPTO_DIR)/certs/vendor-a.chain
	$(ZOOG_ROOT)/toolchains/linux_elf/scripts/binary_to_c_symbols.py $^ vendor_a_chain $(BUILD)/vendor-a-chain.h $(BUILD)/vendor-a-chain.c

$(BUILD)/zoogcpp.o: $(BUILD)/vendor-a-chain.h

$(BUILD)/zhyperoid-pie: $(ZH_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@
