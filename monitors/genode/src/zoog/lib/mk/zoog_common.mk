SRC_CC = \
	ordinary_operator_new.cc \
	SyncFactory_Genode.cc \
	Thread_id.cc \

SRC_C = \
	hash_table.c \
	linked_list.c \
	standard_malloc_factory.c \
	LiteLib.c \
	xax_network_utils.c \

LIBS   = cxx env thread
INC_DIR += $(ABS_ZOOG_ROOT)/common/ifc
INC_DIR += $(ABS_ZOOG_ROOT)/common/utils
INC_DIR += $(ABS_ZOOG_ROOT)/monitors/linux_common/xblit
#INC_DIR += $(REP_DIR)/src/base/lock

CC_OPT = \
	-DZOOG_NO_STANDARD_INCLUDES=1 \

vpath %.cc $(REP_DIR)/src/zoog/common
vpath %.c $(REP_DIR)/src/zoog/common
