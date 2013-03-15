########## Variables

ZOOG_ROOT=../../..

TARGETS=\
	discovery_server \
	discovery_test_client

DEP_SRCS=$(wildcard *.c)

SERVER_SRCS=\
	discovery_server.c \
	discovery_server_args.c \

CLIENT_SRCS=\
	discovery_test_client.c

COMMON_UTILS_MODULE=$(ZOOG_ROOT)/common/utils
COMMON_UTILS_OBJS=\
	LiteLib.o \
	xax_network_utils.o \
	hash_table.o \
	cheesy_snprintf.o \

ELF_COMMON_MODULE=$(ZOOG_ROOT)/toolchains/linux_elf/common/
ELF_COMMON_OBJS=\
	debug_util.o \

MODULES=$(COMMON_UTILS_MODULE) $(ELF_COMMON_MODULE)
MODULE_OBJS=$(COMMON_UTILS_OBJS) $(ELF_COMMON_OBJS)

SERVER_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(SERVER_SRCS)) $(MODULE_OBJS))
CLIENT_OBJS=$(patsubst %,$(BUILD)/%,$(patsubst %.c,%.o,$(CLIENT_SRCS)) $(MODULE_OBJS))

INCS=\
	-I$(BUILD) \
	-I$(ZOOG_ROOT)/toolchains/linux_elf/common \
	-I$(ZOOG_ROOT)/common/ifc \

CFLAGS=\
	-g \
	$(INCS) \

#	-DDEBUG=1 \

########## Include

include ../make.in

########## Rules

$(BUILD)/discovery_server: $(SERVER_OBJS)
	$(CC) $(LIBFLAGS) $^ -o $@

$(BUILD)/discovery_test_client: $(CLIENT_OBJS)
	$(CC) $(LIBFLAGS) $^ -o $@

# TODO should be able to listen for broadcast only on one interface
run: $(BUILD)/discovery_server
	$^ \
	--server_listen_ifc 0.0.0.0 \
	--host_ifc_addr 10.${ZOOG_TUNID}.0.2 \
	--host_ifc_app_range_low 10.${ZOOG_TUNID}.0.3 \
	--host_ifc_app_range_high 10.${ZOOG_TUNID}.0.254 \
	--router_ifc_addr 10.${ZOOG_TUNID}.0.1 \
	--local_zftp_server_addr 10.${ZOOG_TUNID}.0.1 \
	
