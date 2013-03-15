
########## Variables

ZOOG_ROOT=../../../..

TARGETS=cat_zoog_identity

MODULES=
MODULE_OBJS=

CZI_SRCS=\
	cat_zoog_identity.c \

DEP_SRCS=$(CZI_SRCS)

CZI_OBJS=$(call mapSrcsToObjs,$(CZI_SRCS))

INCS=\

CFLAGS=\
	-g -pie -fpie \
	$(INCS) \
	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \

########## Include

include ../make.in

########## Rules

$(BUILD)/cat_zoog_identity: $(CZI_OBJS)
	$(CXX) -pie -g $^ -o $@
