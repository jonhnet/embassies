ZOOG_ROOT=../../../../..

PACKAGE_NAME=libdbus-1-3
NEW_CFLAGS=-g
TARGETS=stamp-post-patched
PRE_PATCH_SCRIPT=\
	patch build/source-files/dbus-1.2.24/debian/rules < verbose.patch

include $(ZOOG_ROOT)/toolchains/linux_elf/apps/make.in
