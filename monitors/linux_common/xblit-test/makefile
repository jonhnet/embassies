
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	xblit-test

LIB_COMMON_MODULE=$(ZOOG_ROOT)/common/lib
LIB_COMMON_OBJS=\

MODULES=$(LIB_COMMON_MODULE)
MODULE_OBJS=$(LIB_COMMON_OBJS)

SRCS=\
	xblit-test.c \

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \
	-I.\

LIBS=\
	-lX11 \
	-lXext \

CFLAGS=\
	-g \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

DEP_SRCS=$(SRCS)

########## Include

include $(ZOOG_ROOT)/toolchains/linux_elf/make.in

########## Rules

$(BUILD)/xblit-test: $(BUILD)/xblit-test.o
	$(CXX) $(CFLAGS) $^ -o $@ $(LIBS)
