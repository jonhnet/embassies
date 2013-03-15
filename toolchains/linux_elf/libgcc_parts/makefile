
########## Variables

ZOOG_ROOT=../../..

TARGETS=_udivdi3.o memcpy.o memcmp.o

#OBJS=$(BUILD)/_udivdi3.o $(BUILD)/memcpy.o

include ../make.in

# cherry-pick udivdi3 out of libgcc, which otherwise we don't want anywhere
# near our code, I think.
GCC_VERSION=$(shell gcc -v 2>&1 | grep "gcc version" | awk '{print $$3}')
$(BUILD)/_udivdi3.o:
	(cd $(BUILD); ar x /usr/lib/gcc/i486-linux-gnu/$(GCC_VERSION)/libgcc.a _udivdi3.o)

$(BUILD)/memcpy.o:
	(cd $(BUILD); ar x $(abspath $(ZOOG_ROOT))/toolchains/linux_elf/libc_base/libc.a memcpy.o)

$(BUILD)/memcmp.o:
	(cd $(BUILD); ar x $(abspath $(ZOOG_ROOT))/toolchains/linux_elf/libc_base/libc.a memcmp.o)
