TARGET = zoog_monitor
SRC_CC = \
	main.cc \
	NetworkThread.cc \
	GBlitProvider.cc \
	GBlitProviderCanvas.cc \
	ZBlitter.cc \
	BlitterCanvas.cc \
	GenodeCanvasAcceptor.cc \
	BlitterViewport.cc \
	BlitterManager.cc \
	UIEventQueue.cc \
	ByteStream.cc \
	crypto.cc \
	CryptoException.cc \
	DomainName.cc \
	PRF.cc \
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
	KeyTranslate.cc \

SRC_C = \
	bignum.c \
	vmac.c \
	sha2big.c \
	sha2small.c \
	rsa.c \

#SRC_BIN = paltest.raw
SRC_BIN = zoog_boot.signed

LIBS   = cxx env server dde_kit dde_ipxe_support dde_ipxe_nic net raw_signal zoog_common blit
INC_DIR += $(ABS_ZOOG_ROOT)/common/ifc
INC_DIR += $(ABS_ZOOG_ROOT)/common/utils
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto
INC_DIR += $(ABS_ZOOG_ROOT)/monitors/common

CC_OPT = \
	-DZOOG_NO_STANDARD_INCLUDES=1 \

