
########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	getaddr \
	getaddr-dbg \

GETADDR_SRCS=\
	getaddr.cpp

MODULES=
MODULE_OBJS=

GETADDR_OBJS=$(call mapSrcsToObjs, $(GETADDR_SRCS) $(MODULE_OBJS))

DEP_SRCS=$(GETADDR_SRCS)

INCS=\
	-I$(ZOOG_ROOT)/common/crypto \
	-I$(ZOOG_ROOT)/common/ifc \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \


########## Include

include ../make.in

########## Rules

$(BUILD)/getaddr: $(GETADDR_OBJS)
	$(CXX) -pie $(CFLAGS) $^ -o $@

$(BUILD)/getaddr-dbg: $(GETADDR_SRCS)
	g++ $^ -g -o $@
