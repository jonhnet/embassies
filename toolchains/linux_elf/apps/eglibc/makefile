ZOOG_ROOT=../../../..

PACKAGE_NAME=eglibc

override_all: build/stamp-built

include ../make.in

$(BUILD)/stamp-built: $(BUILD)/stamp-unpacked
	(cd $(BUILD)/source-files && \
	cd `ls -1 --group-directories-first . | head -1` && \
	dpkg-buildpackage $(PARALLEL_JOBS) -rfakeroot -b > ../build.log 2>&1) || true
	touch $@
