
########## Variables

ZOOG_ROOT=../../..

include ../make.modules

# auto-generated apps: <name>.app.zguest
APPS=gnumeric inkscape xeyes galeon tabman
# custom-configured apps go from <name>.config.c to build <name>.zguest
TARGETS=\
	stamp-objects \
	$(patsubst %,%.app.zguest,$(APPS)) \
	abiword.zguest \
	gnucash.zguest \
	xclock.zguest \
	konqueror.zguest \
	epiphany.zguest \
	midori.zguest \
	marble.zguest \
	hello.zguest \
	navtest.zguest \
	keyval.zguest \
	keyval-test.zguest \
	getaddr.zguest \
	gimp.zguest \

LWIP_LIB_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/lwip_lib
FASTPATH_TEST_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/fastpath_test

PREREQUISITE_DIRS=$(LWIP_LIB_DIR) $(FASTPATH_TEST_DIR)

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	LiteLib.o \
	cheesymalloc.o \
	lite_log2.o \
	format_ip.o \
	cheesy_snprintf.o \
	hash_table.o \
	standard_malloc_factory.o \
	linked_list.o \
	zoog_malloc_factory.o \
	cheesy_malloc_factory.o \
	malloc_factory_operator_new.o \
	ambient_malloc.o \
	pure_virtual.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS= \
	ElfLoad.o \
	ElfArgs.o \
	get_xax_dispatch_table.o \
	xax_util.o \
	packetfilter.o \
	xax_extensions.o \
	concrete_mmap_xdt.o \
	XaxVFS.o \
	AppIdentity.o \

CRYPTO_MODULE_OBJS+=\
	time_posix.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS) $(CRYPTO_MODULE_OBJS)

SRCS=\
	launch_elf_interpreter.c \
	zguest.cpp \
	lwip_module.c \
	Zone.cpp \
	RunConfig.cpp \
	xvnc_start_detector.cpp \
	VncZone.cpp \
	DbusZone.cpp \

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(LWIP_LIB_DIR)/inc \
	-I$(LWIP_LIB_DIR)/build/lwip_release_dir/lwip/src/include \
	-I$(LWIP_LIB_DIR)/build/lwip_release_dir/lwip/src/include/ipv4 \
	-I$(LWIP_LIB_DIR)/build/lwip_release_dir/contrib-1.4.0.rc2/ports/unix/include \
	-I$(LWIP_LIB_DIR)/build/lwip_build_dir \
	-I.\

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-lpthread -ldl \
	-DUSE_FILE_IO=1 \
	-D_GNU_SOURCE \
	-L$(LWIP_LIB_DIR)/build/lwip_build_dir \
	-llwip \

DEP_SRCS=$(SRCS) $(wildcard *.config.c)

########## Include

include ../make.in

########## Rules

%.phonysuffix:
	true
.PHONY: %.phonysuffix

$(BUILD)/%.app.config.cpp: %.phonysuffix
	echo '#include "RunConfig.h"' > $@
	echo 'void config_create(RunConfig *rc) {' >> $@
	echo 'rc->add_vnc();' >> $@
	echo 'rc->add_app("'$(patsubst %.phonysuffix,%,$^)'");' >> $@
	echo '}' >> $@
.PRECIOUS:$(BUILD)/%.app.config.cpp

# All of $(OBJS) must exist for %.zguest pattern to match,
# if we declare a dependency on them. We certainly
# don't want to omit the dependency, so the alternative is
# to ensure that the $(OBJS) get built before we try to match
# the pattern.

$(BUILD)/stamp-objects: $(OBJS)
	touch $@

$(BUILD)/%.zguest: $(BUILD)/%.config.o $(OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

