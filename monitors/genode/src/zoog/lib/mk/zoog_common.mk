COMMON_CRYPTO_CC = \
	crypto.cc \
	CryptoException.cc \
	DomainName.cc \
	ZBinaryRecord.cc \
	ZCertChain.cc \
	ZCert.cc \
	ZDelegationRecord.cc \
	ZKey.cc \
	ZKeyLinkRecord.cc \
	ZKeyPair.cc \
	ZPrivKey.cc \
	ZPubKey.cc \
	ZPubKeyRecord.cc \
	ZRecord.cc \
	ZSigRecord.cc \
	ZSymKey.cc \
	rsa.cc \
	RandomSeed.cc \
	RandomSupply.cc \
	getCurrentTimeGenode.cc \
	bignum.cc \

SRC_CC = \
	ordinary_operator_new.cc \
	SyncFactory_Genode.cc \
	Thread_id.cc \
	$(COMMON_CRYPTO_CC) \

COMMON_CRYPTO_C = \
	vmac.c \
	sha2big.c \
	sha2small.c \
	rijndael.c \
	sha1.c \

SRC_C = \
	hash_table.c \
	linked_list.c \
	standard_malloc_factory.c \
	LiteLib.c \
	xax_network_utils.c \
	$(COMMON_CRYPTO_C) \

LIBS   = cxx env thread
INC_DIR += $(ABS_ZOOG_ROOT)/common/ifc
INC_DIR += $(ABS_ZOOG_ROOT)/common/utils
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto-patched/patched
INC_DIR += $(ABS_ZOOG_ROOT)/monitors/linux_common/xblit
#INC_DIR += $(REP_DIR)/src/base/lock

CC_OPT = \
	-DZOOG_NO_STANDARD_INCLUDES=1 \

vpath %.cc $(REP_DIR)/src/zoog/common
vpath %.c $(REP_DIR)/src/zoog/common
