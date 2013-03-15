really-all:
	@echo This isn't just a debug lib, it's a required standard component,
	@echo which we need to postprocess to use in zoog.
	@echo Hence, moved to apps/eglibc

ZOOG_ROOT=../../../../..

PACKAGE_NAME=libc6
NEW_CFLAGS=-g
TARGETS=stamp-post-patched

include $(ZOOG_ROOT)/toolchains/linux_elf/apps/make.in
