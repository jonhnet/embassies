ZOOG_ROOT=../../../../..

PACKAGE_NAME=libgc1c2
NEW_CFLAGS=-g
TARGETS=stamp-post-patched

PRE_PATCH_SCRIPT=\
	patch ./build/source-files/libgc-6.8/os_dep.c < os_dep.patch

include $(ZOOG_ROOT)/toolchains/linux_elf/apps/make.in
