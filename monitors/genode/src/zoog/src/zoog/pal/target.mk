TARGET = zoog_pal
SRC_CC = \
	main.cc \
	CoalescingAllocator.cpp \
	abort.cc \
	Genode_pal.cc \
	genode_thread_test.cc \
	ThreadTable.cc \
	ZoogThread.cc \
	ZutexWaiter.cc \
	ZutexTable.cc \
	ZutexQueue.cc \
	ZoogAlarmThread.cc \
	HostAlarms.cc \
	NetBufferTable.cc \
	CoreWriter.cc \
	MonitorAlarmThread.cc \
	DebugServer.cc \
	zoog_qsort.cc \
	ChannelWriter.cc \
	CanvasDownsampler.cc \

SRC_C = \
	corefile.c \

LIBS   = cxx env thread x86_segments zoog_common raw_signal
INC_DIR += $(ABS_ZOOG_ROOT)/common/ifc
INC_DIR += $(ABS_ZOOG_ROOT)/common/utils
INC_DIR += $(ABS_ZOOG_ROOT)/common/crypto
INC_DIR += $(ABS_ZOOG_ROOT)/monitors/common
INC_DIR += $(REP_DIR)/src/zoog/common
INC_DIR += $(REP_DIR)/src/zoog/pal
	# for core_regs.h, included from far away.
#INC_DIR += $(REP_DIR)/src/base/lock

CC_OPT = \
	-DZOOG_NO_STANDARD_INCLUDES=1 \
	-DZOOG_ROOT=\"$(ABS_ZOOG_ROOT)\" \

#	-D__bool_true_false_are_defined=1 \
