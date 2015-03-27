# TODO: relieve "makefile:12: warning: overriding commands for target `clean'" warning

MAKEDIRS=monitors toolchains/linux_elf
all:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} ;\
	done
	
clean:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
