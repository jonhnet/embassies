
########## Variables

include ../make.arch

ZOOG_ROOT=../../..

TARGETS=exceptions_test

MODULES=
MODULES_OBJS=

SRCS=\
	exceptions_test.cpp \

DEP_SRCS=$(SRCS)

OBJS=$(call mapSrcsToObjs,$(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ELF_FLATTEN_DIR) \

CFLAGS=\
	-g -static \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-D_GNU_SOURCE \
	-DZFTP_USE_OPENSSL \
	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

########## Include

include ../make.in

########## Rules

$(BUILD)/exceptions_test: $(OBJS)
	$(CXX) $^ -o $@
