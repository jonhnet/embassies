
########## Variables

ZOOG_ROOT=../../..

include ../make.modules

TARGETS=\
	hash_table_test \
	core_file_test \
	my-core \
	ordered_collection_test \
	linked_list_test \
	binsearch_test \
	page_allocator_test \
	decode_ipv4_test \
	decode_ipv6_test \
	snprintf_test \
	quicksort_test \
	format_ip_test \
	avl_tree_test \
	coalescing_allocator_test \
	throughput_test \
	throughput_client \
	memcpy_test \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	corefile.o \
	page_allocator.o \
	alignator.o \
	lite_log2.o \
	xax_network_utils.o \
	zoog_qsort.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	standard_malloc_factory.o \

ZOOG_COMMON_MODULE=$(ZOOG_ROOT)/common/lib

ZOOG_COMMON_OBJS=\
	LiteLib.o \
	malloc_factory.o \
	hash_table.o \
	ordered_collection.o \
	cheesylock.o \
	linked_list.o \
	binsearch.o \
	concrete_mmap_posix.o \
	cheesy_snprintf.o \
	xax_util.o \
	format_ip.o \

COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common

IFC_MODULE=$(ZOOG_ROOT)/common/ifc

MODULES=$(ZOOG_COMMON_MODULE) $(ELF_COMMON_MODULE) $(COMMON_UTILS_MODULE) $(IFC_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(ZOOG_COMMON_OBJS) $(ELF_COMMON_OBJS) $(COMMON_UTILS_OBJS)

HT_SRCS= hash_table_test.c \

CF_SRCS= core_file_test.c \

OC_SRCS= ordered_collection_test.c \

LL_SRCS= linked_list_test.c \

BS_SRCS= binsearch_test.c \

PA_SRCS= page_allocator_test.c \

DI_SRCS= decode_ipv4_test.c \

D6_SRCS= decode_ipv6_test.c \

SP_SRCS= snprintf_test.c \

QS_SRCS= quicksort_test.c \

FI_SRCS= format_ip_test.c \

AV_SRCS= avl_tree_test.c \

AV_MODULE_OBJS=\
	malloc_factory_operator_new.o \
	standard_malloc_factory.o \
	SyncFactory_Pthreads.o \
	pure_virtual.o \

CA_SRCS= coalescing_allocator_test.c \

CA_MODULE_OBJS=\
	malloc_factory_operator_new.o \
	standard_malloc_factory.o \
	SyncFactory_Pthreads.o \
	pure_virtual.o \
	CoalescingAllocator.o \

TP_SRCS= throughput_test.c \

TC_SRCS= throughput_client.c \

SRCS=$(HT_SRCS) $(CF_SRCS) $(OC_SRCS) $(LL_SRCS) $(BS_SRCS) $(PA_SRCS) $(SP_SRCS) $(QS_SRCS) $(FI_SRCS) $(AV_SRCS) $(CA_SRCS) $(TP_SRCS) $(TC_SRCS)

DEP_SRCS=$(SRCS)

# NB elf_start.o must appear first, so that its function definition is
# the first executable byte in the file, so that when we strip off the
# Elf header, we have a program with an entry point at file offset 0.
HT_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(HT_SRCS) $(MODULE_OBJS)))
CF_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(CF_SRCS) $(MODULE_OBJS)))
OC_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(OC_SRCS) $(MODULE_OBJS)))
LL_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(LL_SRCS) $(MODULE_OBJS)))
BS_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(BS_SRCS) $(MODULE_OBJS)))
PA_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(PA_SRCS) $(MODULE_OBJS)))
DI_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(DI_SRCS) $(MODULE_OBJS)))
D6_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(D6_SRCS) $(MODULE_OBJS)))
SP_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(SP_SRCS) $(MODULE_OBJS)))
QS_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(QS_SRCS) $(MODULE_OBJS)))
FI_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(FI_SRCS) $(MODULE_OBJS)))
AV_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(AV_SRCS) $(MODULE_OBJS) $(AV_MODULE_OBJS)))
CA_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(CA_SRCS) $(MODULE_OBJS) $(CA_MODULE_OBJS)))
TP_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(TP_SRCS) $(MODULE_OBJS)))
TC_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(TC_SRCS) $(MODULE_OBJS)))

INCS=\
	-I$(ZOOG_COMMON_MODULE) \
	-I$(ELF_COMMON_MODULE) \
	-I$(BUILD) \
	-DUSE_FILE_IO=1 \

CFLAGS=\
	-g \
	$(INCS) \
	-DUNIT_TEST \

########## Include

include ../make.in

########## Rules

$(BUILD)/hash_table_test: $(HT_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/core_file_test: $(CF_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/ordered_collection_test: $(OC_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/linked_list_test: $(LL_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/binsearch_test: $(BS_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/page_allocator_test: $(PA_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/decode_ipv4_test: $(DI_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/decode_ipv6_test: $(D6_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/snprintf_test: $(SP_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/quicksort_test: $(QS_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/format_ip_test: $(FI_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/avl_tree_test: $(AV_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ -lpthread

$(BUILD)/coalescing_allocator_test: $(CA_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ -lpthread

$(BUILD)/throughput_test: $(TP_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/throughput_client: $(TC_OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

$(BUILD)/corefile.o: $(BUILD)/local_elf.h

$(BUILD)/local_elf.h: /usr/include/elf.h
	grep -v '#include <features.h>' < $< > $@

$(BUILD)/my-core: $(BUILD)/core_file_test
	$<
