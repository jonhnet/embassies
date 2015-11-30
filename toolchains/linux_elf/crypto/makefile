########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	crypto_util \
	crypto_test \
	dev_principal.local.cert \
	pair_to_pub \
	cert_to_hash \
	vmac_speed_test \

DEVELOPER_PRINCIPAL=`whoami`.`hostname`

CRYPTO_UTIL_SRCS=\
  crypto_util.cpp

CRYPTO_TEST_SRCS=\
  crypto_test.cpp

CERT_TO_HASH_SRCS=\
  cert_to_hash.cpp

PAIR_TO_PUB_SRCS=\
  pair_to_pub.cpp

VMAC_SPEED_TEST_SRCS=\
	vmac_speed_test.cpp \
	GTimer.cpp \

DEP_SRCS=\
	$(CRYPTO_UTIL_SRCS) \
	$(CRYPTO_TEST_SRCS) \
	$(CERT_TO_HASH_SRCS) \
	$(PAIR_TO_PUB_SRCS) \
	$(VMAC_SPEED_TEST_SRCS) \

########## Include more modules

include ../make.modules


# Project-specific module objects
CRYPTO_MODULE_OBJS += \
  tests.o \
  ZApp.o \
  timer.o \

#COMMON_UTILS_OBJS += standard_malloc_factory.o

##########

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
  ambient_malloc.o \
  malloc_factory.o \
  LiteLib.o \
  standard_malloc_factory.o \
  hash_to_hex.o \

########## Rules

MODULES=$(COMMON_UTILS_MODULE) $(CRYPTO_MODULE) 
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(CRYPTO_MODULE_OBJS) 

CRYPTO_UTIL_OBJS=$(call mapSrcsToObjs,$(CRYPTO_UTIL_SRCS) $(MODULE_OBJS))
CRYPTO_TEST_OBJS=$(call mapSrcsToObjs,$(CRYPTO_TEST_SRCS) $(MODULE_OBJS))
CERT_TO_HASH_OBJS=$(call mapSrcsToObjs,$(CERT_TO_HASH_SRCS) $(MODULE_OBJS))
PAIR_TO_PUB_OBJS=$(call mapSrcsToObjs,$(PAIR_TO_PUB_SRCS) $(MODULE_OBJS))
VMAC_SPEED_TEST_OBJS=$(call mapSrcsToObjs,$(VMAC_SPEED_TEST_SRCS) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=\
	$(INCS) \
	-O3 \
	-DUSE_CPPSTDLIB=1 \
  -fexceptions \
	-march=native \
#	-g \
	#-O2 \
	#-O3 \
	#-march=pentium-m \
	#-g \
	#-fno-strict-aliasing \

#LIBFLAGS=\
#  -L $(ZOOG_ROOT)/common/crypto/lib 

#Uncomment the following to test with NaCl crypto library
#LIBFLAGS=$(LIBFLAGS) -lnacl
#CFLAGS=$(CFLAGS) -DNACL

########## Include

include ../make.in

########## Rules

$(BUILD)/crypto_util: $(CRYPTO_UTIL_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/crypto_test: $(CRYPTO_TEST_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/cert_to_hash: $(CERT_TO_HASH_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/pair_to_pub: $(PAIR_TO_PUB_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/vmac_speed_test: $(VMAC_SPEED_TEST_OBJS)
	$(CXX) -o $@ $^ $(LIBFLAGS)

$(BUILD)/dev_principal.local.keys: $(BUILD)/crypto_util
	$(BUILD)/crypto_util --genkeys $@ --name $(DEVELOPER_PRINCIPAL)

keys/root.keys:
	./regenerate-keys-certs.sh

$(BUILD)/dev_principal.local.cert: $(BUILD)/dev_principal.local.keys keys/root.keys
	$(BUILD)/crypto_util \
		--ekeypair $< \
		--name $(DEVELOPER_PRINCIPAL) \
		--cert $@ \
		--skeypair keys/root.keys \
		--inception `date +%s` \
		--expires `echo \`date +%s\` + 31536000 | bc `


