
########## Variables

ZOOG_ROOT=../../..

TARGETS=proxytest

MODULES=
MODULE_OBJS=

SRCS=\
	proxytest.c \

OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(SRCS) $(MODULE_OBJS)))

INCS=\
	-I$(BUILD) \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
	-lpthread \

DEP_SRCS=$(SRCS)

########## Include

include ../make.in

########## Rules

$(BUILD)/proxytest: $(OBJS)
	$(CC) -pie $(CFLAGS) $^ -o $@
