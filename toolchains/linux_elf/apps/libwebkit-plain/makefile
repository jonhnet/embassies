ZOOG_ROOT=../../../..

PACKAGE_NAME=libwebkit-1.0-2
NEW_CFLAGS=-g
TARGETS=libwebkit-1.0.so.2.stripped

PRE_PATCH_SCRIPT=\

include ../make.in

# make both unstripped & stripped variants
WEBKIT_LIB=$(BUILD)/source-files/webkit-1.2.7/build/.libs/libwebkit-1.0.so.2

$(WEBKIT_LIB): $(BUILD)/stamp-post-patched

$(BUILD)/libwebkit-1.0.so.2.stripped: $(BUILD)/stamp-post-patched
	strip $(WEBKIT_LIB) -o $@
