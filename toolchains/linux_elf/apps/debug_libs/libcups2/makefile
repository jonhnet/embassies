ZOOG_ROOT=../../../../..

PACKAGE_NAME=libcups2
NEW_CFLAGS=-g
TARGETS=stamp-post-patched

PRE_PATCH_SCRIPT=\
	patch build/source-files/cups-*/debian/rules < rules.patch

include $(ZOOG_ROOT)/toolchains/linux_elf/apps/make.in
