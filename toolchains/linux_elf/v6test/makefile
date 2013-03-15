
########## Variables

ZOOG_ROOT=../../..

TARGETS=v6test

MODULES=
MODULE_OBJS=

SRCS=\
	v6test.c \

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\

CFLAGS=\
	-g \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

DEP_SRCS=$(SRCS)

########## Include

include ../make.in

########## Rules

$(BUILD)/v6test: $(OBJS)
	$(CXX) $(CFLAGS) $^ -o $@
