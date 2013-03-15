########## Variables

ZOOG_ROOT=../../..

TARGETS=failsafe_experiment

ZOOG_COMMON_MODULE=$(ZOOG_ROOT)/common/lib
ZOOG_COMMON_MODULE_OBJS=\

MODULES=$(ZOOG_COMMON_MODULE)
MODULE_OBJS=$(ZOOG_COMMON_MODULE_OBJS)

SRCS=\
	failsafe_experiment.c \

DEP_SRCS=$(SRCS)

OBJS=$(patsubst %.o,$(BUILD)/%.o,$(patsubst %.c,%.o,$(SRCS)) $(MODULE_OBJS))

INCS=\
	-I$(ZOOG_ROOT)/common/ifc \

LIBS=\
	-lpthread\

CFLAGS=\
	-g \
	$(INCS) \
	$(LIBS) \

#	-DDEBUG_ZUTEX_OWNER=1 \

########## Include

include ../make.in

########## Rules

$(BUILD)/failsafe_experiment: $(OBJS)
	$(CC) $^ -o $@
