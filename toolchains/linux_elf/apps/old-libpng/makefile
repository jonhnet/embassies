ZOOG_ROOT=../../../..

FETCH=$(ZOOG_ROOT)/toolchains/linux_elf/scripts/fetch-code.py
OLD_LIBPNG_FILENAME=libpng12-0_1.2.44-1+squeeze1_i386.deb
OLD_LIBPNG_URL=http://packages.vyatta.com/debian-security/pool/updates/main/libp/libpng/$(OLD_LIBPNG_FILENAME)
CACHE_PATH=$(ZOOG_ROOT)/external-code/old-libpng/

BUILD=./build

all: $(BUILD)/overlay

clean:
	rm -rf $(BUILD)

$(BUILD)/$(OLD_LIBPNG_FILENAME):
	mkdir -p $(BUILD)
	$(FETCH) $@ $(OLD_LIBPNG_URL) $(CACHE_PATH)

$(BUILD)/overlay: $(BUILD)/$(OLD_LIBPNG_FILENAME)
	mkdir -p $@
	dpkg -x $< $@
