# TODO: relieve "makefile:12: warning: overriding commands for target `clean'" warning

MAKEDIRS=../common/crypto-patched linux_dbg linux_kvm
all:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} ;\
	done
	
clean:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
