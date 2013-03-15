
########## Variables

ZOOG_ROOT=../../..

include ../make.modules

TARGETS=\
	keyval-pie \
	keyval-test-pie \

KEYVAL_SRCS=\
	keyval_zoog.cpp \

TEST_SRCS=\
	test_client.cpp \

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	hash_table.o \
	standard_malloc_factory.o \
	LiteLib.o

MODULES=$(COMMON_UTILS_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS)

KEYVAL_OBJS=$(call mapSrcsToObjs, $(KEYVAL_SRCS) $(MODULE_OBJS))
TEST_OBJS=$(call mapSrcsToObjs, $(TEST_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(KEYVAL_SRCS) $(TEST_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/ifc/ \
	-I$(ZOOG_ROOT)/common/keyval_module/ \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \


########## Include

include ../make.in

########## Rules

$(BUILD)/keyval-pie: $(KEYVAL_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

$(BUILD)/keyval-test-pie: $(TEST_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

