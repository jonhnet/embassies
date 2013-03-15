########## Variables

ZOOG_ROOT=../../..

TARGETS=liblwip.so.stamp

DEP_SRCS=$(wildcard *.c)

# none of the handy make.in stuff does anything useful for us,
# since we're running the Makefile that comes with lwip (with hard-coded
# patches).

INCS=\
	-I$(ZOOG_ROOT)/common/ifc \
	-IBOOG

CFLAGS=\
	-g -fpic -static -nostdlib \
	$(INCS) \
	-DUSE_OPENSSL

LWIP_DOWNLOAD_SITE=http://download.savannah.gnu.org/releases/lwip/
#LWIP_DOWNLOAD_SITE=http://ftp.twaren.net/Unix/NonGNU/lwip/
#LWIP_DOWNLOAD_SITE=http://nongnu.askapache.com/lwip/
LWIP_VERSION=-1.4.0.rc2
EXTERNAL_CODE=$(ZOOG_ROOT)/external-code
FETCH=$(ZOOG_ROOT)/toolchains/linux_elf/scripts/fetch-code.py

########## Include

include ../make.in

########## Rules

new_patch:
	diff -u $(BUILD)/lwip_release_dir/contrib-1.4.0.rc2/ports/unix/proj/lib/Makefile $(BUILD)/lwip_build_dir/Makefile > new_patch || true

$(BUILD)/lwip_release_dir/unpack.stamp:
	mkdir -p build/lwip_release_dir
	$(FETCH) build/lwip_release_dir/lwip$(LWIP_VERSION).zip $(LWIP_DOWNLOAD_SITE)lwip$(LWIP_VERSION).zip $(EXTERNAL_CODE)/lwip/lwip$(LWIP_VERSION).zip
	(cd build/lwip_release_dir && unzip -q lwip$(LWIP_VERSION))
	$(FETCH) build/lwip_release_dir/contrib$(LWIP_VERSION).zip $(LWIP_DOWNLOAD_SITE)contrib$(LWIP_VERSION).zip $(EXTERNAL_CODE)/lwip/contrib$(LWIP_VERSION).zip
	(cd build/lwip_release_dir && unzip -q contrib$(LWIP_VERSION))
	(cd build/lwip_release_dir && ln -s lwip$(LWIP_VERSION) lwip)
	touch $@

#	./fetch_lwip.py $(LWIP_DOWNLOAD_SITE) $(dir $@) $(LWIP_VERSION)

$(BUILD)/lwip_build_dir/patch.stamp: $(BUILD)/lwip_release_dir/unpack.stamp
	rm -rf $(dir $@)
	mkdir -p $(dir $@)
	#
	cp $(BUILD)/lwip_release_dir/contrib-1.4.0.rc2/ports/unix/proj/lib/Makefile $(dir $@)
	patch $(dir $@)/Makefile contrib_unix_makefile.patch
	#
	cp $(BUILD)/lwip_release_dir/contrib-1.4.0.rc2/ports/unix/sys_arch.c $(dir $@)/unix_arch.c
	patch $(dir $@)/unix_arch.c unix_arch.c.patch
	#
	cp $(BUILD)/lwip_release_dir/contrib-1.4.0.rc2/ports/unix/proj/unixsim/lwipopts.h $(dir $@)/lwipopts.h
	patch $(dir $@)/lwipopts.h lwipopts.h.patch
	#
	cp $(BUILD)/lwip_release_dir/lwip-1.4.0.rc2/src/api/sockets.c $(dir $@)/sockets.c
	patch $(dir $@)/sockets.c sockets.c.patch
	#
	touch $@

$(BUILD)/liblwip.so.stamp: $(BUILD)/lwip_build_dir/patch.stamp
	$(MAKE) -C $(BUILD)/lwip_build_dir
	touch $@
