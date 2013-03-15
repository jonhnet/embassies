ZOOG_ROOT=../../../..

PACKAGE_NAME=midori
PIE_NAME=midori-pie

LINUX_ELF_COMMON=$(ZOOG_ROOT)/toolchains/linux_elf/common
LINUX_ELF_COMMON_FULL=$(realpath $(LINUX_ELF_COMMON))

PRE_PATCH_SCRIPT=\
	patch build/source-files/midori-*/debian/rules < src/waf-link-pie.patch && \
	patch -d build/source-files/midori-0.2.4/midori < src/timing.patch && \
	patch build/source-files/midori-*/midori/main.c < src/retry-history-db.patch && \
	(cd $(BUILD)/source-files/midori-0.2.4/midori; ln -s $(LINUX_ELF_COMMON_FULL)/xax_extensions_base.h . ) && \
	(cd $(BUILD)/source-files/midori-0.2.4/midori; ln -s $(LINUX_ELF_COMMON_FULL)/xe_mark_perf_point.h . ) && \
	(cd $(BUILD)/source-files/midori-0.2.4/midori; ln -s $(LINUX_ELF_COMMON_FULL)/xe_mark_perf_point.c . ) && \
	true

BUILT_PIE=build/source-files/midori-*/debian/midori/usr/bin/midori

include ../make.in

TARGETS += native-midori-timing

$(BUILD)/native-midori-timing: src/native-midori-timing.c
	gcc -g $^ -o $@
