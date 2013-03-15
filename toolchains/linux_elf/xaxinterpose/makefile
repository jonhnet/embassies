########## Variables

ZOOG_ROOT=../../..

TARGETS=libxaxinterpose.a

DEP_SRCS=$(wildcard *.c) $(wildcard *.S)

LIB_SRCS=\
	$(DEP_SRCS)

COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
COMMON_OBJS=
MODULES=$(COMMON_MODULE)
MODULE_OBJS=$(COMMON_OBJS)

LIB_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(LIB_SRCS) $(MODULE_OBJS))))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=\
	-g -fpic -static -nostdlib \
	$(INCS)

########## Include

include ../make.in

########## Rules

$(BUILD)/libxaxinterpose.a: $(LIB_OBJS)
	rm -f $@
	ar rc $@ $^

