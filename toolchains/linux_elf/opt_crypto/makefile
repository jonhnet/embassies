
########## Variables

include ../make.arch

ZOOG_ROOT=../../..

TARGETS=\
	libopt_crypto.a

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils

include ../make.modules

MODULES= $(COMMON_UTILS_MODULE) $(CRYPTO_LIB_MODULE) $(CRYPTO_MODULE)
MODULE_OBJS= $(CRYPTO_LIB_MODULE_OBJS) $(CRYPTO_MODULE_OBJS)

SRCS=

DEP_SRCS=

OBJS=$(call mapSrcsToObjs, $(SRCS) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/common/ifc \

NOSTDLIB=-nostdlib -fno-exceptions

CFLAGS=\
	-g -O3 -pie -fpie -static $(NOSTDLIB) \
	$(INCS) \
	-DELFOBJ_USE_LIBC=0 \
	-DNOEXCEPTIONS \
	-march=native \

#	-D_GNU_SOURCE \
#	-DZOOG_ROOT='"$(realpath $(ZOOG_ROOT))"' \
#	-DSTATIC_WORD_SIZE=$(STATIC_WORD_SIZE) \

########## Include

include ../make.in

########## Rules

.SECONDARY:

$(BUILD)/libopt_crypto.a: $(OBJS)
	ar rcs $@ $^
