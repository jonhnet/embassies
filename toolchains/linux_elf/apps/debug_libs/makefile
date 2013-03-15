ZOOG_ROOT=../../../..

DEBUG_LIB_DIRS=$(dir $(wildcard */makefile))

PREREQUISITE_DIRS=$(DEBUG_LIB_DIRS)

TARGETS=

include ../../make.in

stamp-unpacked:
	@for dir in $(PREREQUISITE_DIRS); do \
		$(MAKE) -C $${dir} $(BUILD)/stamp-unpacked ;\
	done
.PHONY:stamp-unpacked

clean:
	@for dir in $(PREREQUISITE_DIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
.PHONY:clean
