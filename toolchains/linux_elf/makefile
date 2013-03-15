PREREQUISITE_DIRS=install_deps apps/eglibc ../../common/crypto-patched elf_loader zguest scripts writedoctest paltest unit_tests zftp_backend zftp_zoog zftp_create_zarfile zoog_tests navtest tabman keyval_zoog leanloader measure_posix apps/old-libpng

# s3fs -- build mysteriously broke.

include ./make.in

# ln -s apps/eglibc/build/source-files/eglibc-2.11.2/build-tree/i386-libc/ libc_base

# TODO: relieve "makefile:12: warning: overriding commands for target `clean'" warning

MAKEDIRS=$(filter-out apps/,$(dir $(wildcard */makefile)))
clean:
	@for dir in $(MAKEDIRS); do \
		$(MAKE) -C $${dir} clean ;\
	done
