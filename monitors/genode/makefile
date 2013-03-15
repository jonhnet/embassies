ZOOG_ROOT=../..
export ZOOG_ROOT

PREREQUISITE_DIRS=

include $(ZOOG_ROOT)/toolchains/linux_elf/make.in

ABS_ZOOG_ROOT=$(abspath $(ZOOG_ROOT))
export ABS_ZOOG_ROOT

all: genode-build

genode-toolchain: /usr/local/genode-gcc/bin/genode-x86-gcc

/usr/local/genode-gcc/bin/genode-x86-gcc:
	sudo tar -P -xjf $(ZOOG_ROOT)/external-code/genode/genode-toolchain-11.11.tar.bz2

$(BUILD)/unpack.stamp:
	rm -rf $(BUILD)
	mkdir $(BUILD)
	tar -C $(BUILD) -xjf $(ZOOG_ROOT)/external-code/genode/genode-11.11.tar.bz2
	patch $(BUILD)/genode-11.11/base/src/base/lock/lock.cc < patches/lock.cc.patch
	patch $(BUILD)/genode-11.11/base/include/base/lock.h < patches/lock.h.patch
	(cd $(BUILD)/genode-11.11/base/; patch -p4) < patches/base_x86_segments.patch
	(cd $(BUILD)/genode-11.11; patch -p0) < patches/malloc_free.cc.patch
	make -C $(BUILD)/genode-11.11/base-pistachio prepare
	make -C $(BUILD)/genode-11.11/dde_ipxe prepare
	(cd $(BUILD)/genode-11.11/base-pistachio/; patch -p1) < patches/pistachio_x86_segments.patch
	echo disabled: make -C $(BUILD)/genode-11.11/ports prepare PKG=gdb
	(cd $(BUILD)/genode-11.11; ln -s ../../src/zoog zoog)
	touch $@

# patch $(BUILD)/genode-11.11/base/lib/mk/cxx.mk < patches/cxx.mk.patch

$(BUILD)/make-genode-build-dir.stamp: $(BUILD)/unpack.stamp
	(cd $(BUILD)/genode-11.11; ./tool/create_builddir pistachio_x86 BUILD_DIR=build.pistachio_x86)
	echo 'REPOSITORIES += $$(GENODE_DIR)/dde_ipxe' >> $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf
	echo 'REPOSITORIES += $$(GENODE_DIR)/zoog' >> $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf
	echo 'REPOSITORIES += $$(GENODE_DIR)/ports' >> $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf
	echo 'MAKE += -j2' >> $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf
	grep -v QEMU_OPT < $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf > $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf.tmp
	mv $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf.tmp $(BUILD)/genode-11.11/build.pistachio_x86/etc/build.conf
	touch $@

$(BUILD)/make-genode-linux-build-dir.stamp: $(BUILD)/unpack.stamp
	(cd $(BUILD)/genode-11.11; ./tool/create_builddir linux_x86 BUILD_DIR=build.linux_x86)
	echo 'REPOSITORIES += $$(GENODE_DIR)/zoog' >> $(BUILD)/genode-11.11/build.linux_x86/etc/build.conf
	touch $@

genode-build: $(BUILD)/make-genode-build-dir.stamp genode-toolchain
	make -C $(BUILD)/genode-11.11/build.pistachio_x86 run/zoog

genode-linux-build: $(BUILD)/make-genode-linux-build-dir.stamp genode-toolchain
	make -C $(BUILD)/genode-11.11/build.linux_x86 run/zoog
