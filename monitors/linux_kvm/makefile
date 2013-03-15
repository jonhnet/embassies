ZOOG_ROOT=../..

PREREQUISITE_DIRS=pal monitor coordinator

include ./make.in

# TODO: relieve "makefile:12: warning: overriding commands for target `clean'" warning

MAKEDIRS=$(dir $(wildcard */makefile))
clean:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
