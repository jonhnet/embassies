ZOOG_ROOT=../../../..

PACKAGE_NAME=gnumeric
PIE_NAME=gnumeric-pie

TARGETS=$(PIE_NAME) libspreadsheet.so

BUILT_PIE=build/source-files/gnumeric-*/build/src/.libs/gnumeric

include ../make.in

$(BUILD)/libspreadsheet.so: $(BUILD)/$(PIE_NAME)
	cp build/source-files/gnumeric-*/build/src/.libs/libspreadsheet.so $@
