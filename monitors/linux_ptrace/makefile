ZOOG_ROOT=../..

PREREQUISITE_DIRS=failsafe_experiment

include ./make.in

# TODO: relieve "makefile:12: warning: overriding commands for target `clean'" warning

MAKEDIRS=$(dir $(wildcard */makefile))
clean:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
