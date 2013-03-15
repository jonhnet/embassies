ZOOG_ROOT=../../../..

PACKAGE_NAME=x11-apps
PIE_NAME=xeyes-pie

TARGETS=$(PIE_NAME) xclock-pie

# TODO figure out how to find this path reliably
#BUILT_PIE=build/source-files/x11-apps-*/xeyes-build/xeyes
BUILT_PIE=build/source-files/x11-apps-*/debian/x11-apps/usr/bin/xeyes

include ../make.in

# TODO copy-pasting code from make.in, because it doesn't currently
# have a clean way to specify extracting multiple executables.
$(BUILD)/xclock-pie: $(BUILD)/stamp-post-patched $(PRUNE_ELF_DYNAMIC_PATH)
	$(PRUNE_ELF_DYNAMIC_PATH) $(wildcard build/source-files/x11-apps-*/debian/x11-apps/usr/bin/xclock) $@

