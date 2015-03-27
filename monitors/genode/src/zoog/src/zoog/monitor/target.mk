TARGET = zoog_monitor

SRC_CC = \
	main.cc \
	root_component.cc \
	session_component.cc \
	boot_block.cc \
	PacketQueue.cc \
	NetworkThread.cc \
	NetworkTerminus.cc \
	GBlitProvider.cc \
	GBlitProviderCanvas.cc \
	ZBlitter.cc \
	BlitterCanvas.cc \
	GenodeCanvasAcceptor.cc \
	BlitterViewport.cc \
	BlitterManager.cc \
	ViewportHandle.cc \
	DeedEntry.cc \
	UIEventQueue.cc \
	GrowBuffer.cc \
	ByteStream.cc \
	ZRectangle.cc \
	KeyTranslate.cc \
	ChannelWriter.cc \
	AllocatedBuffer.cc \

SRC_C = \

#SRC_BIN = paltest.raw
SRC_BIN = zoog_boot.signed zoog_root_key

LIBS   = cxx env server dde_kit dde_ipxe_support dde_ipxe_nic net raw_signal zoog_common blit
INC_DIR += $(ABS_ZOOG_ROOT)/common/ifc
INC_DIR += $(ABS_ZOOG_ROOT)/common/utils
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto-patched/patched
INC_DIR += $(ABS_ZOOG_ROOT)/monitors/common
INC_DIR += $(REP_DIR)/src/zoog/common

CC_OPT = \
	-DZOOG_NO_STANDARD_INCLUDES=1 \

