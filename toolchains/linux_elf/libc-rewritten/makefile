
########## Variables

ZOOG_ROOT=../../..
LIBC_BASE=../libc_base/
STATIC_REWRITER_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/static_rewriter/
STATIC_REWRITER=$(STATIC_REWRITER_DIR)/build/static_rewriter
MAKE_BSS_REAL_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/make_bss_real/
EGLIBC_DIR=$(ZOOG_ROOT)/toolchains/linux_elf/apps/eglibc
LD_BSS=$(MAKE_BSS_REAL_DIR)/build/ld_bss.so

PREREQUISITE_DIRS=$(MAKE_BSS_REAL_DIR) $(STATIC_REWRITER_DIR)
# $(EGLIBC_DIR)

#LD_SO=elf/ld.so

INPUT_LIBS= \
	libc.so \
	rt/librt.so \
	nptl/libpthread.so \
	elf/ld.so \
	dlfcn/libdl.so \

INPUT_LIB_PATHS=$(patsubst %,$(LIBC_BASE)/%,$(INPUT_LIBS)) \
	$(MAKE_BSS_REAL_DIR)/build/ld_bss.so

TARGETS=$(patsubst %.so,%.sw.so,$(notdir $(INPUT_LIB_PATHS)))

# we don't actually *have* any .c's, but just for consistency we'll
# define DEP_SRCS here in the conventional way.
DEP_SRCS=$(wildcard *.c)

#	resolv/libanl.so \
#	resolv/libnss_dns.so \
#	resolv/libresolv.so \
#	nptl_db/libthread_db.so \
#	login/libutil.so \
#	malloc/libmemusage.so \
#	crypt/libcrypt.so \
#	locale/libBrokenLocale.so \
#	debug/libpcprofile.so \
#	debug/libSegFault.so \
#	nis/libnss_compat.so \
#	nis/libnss_nisplus.so \
#	nis/libnss_nis.so \
#	nis/libnsl.so \
#	dlfcn/libdl.so \
#	hesiod/libnss_hesiod.so \

########## Include

include ../make.in

########## Rules

$(BUILD)/libc.sw.so: $(LIBC_BASE)/libc.so $(STATIC_REWRITER)
	$(STATIC_REWRITER) \
		--ignore-func __GI_strrchr \
		--input $< --output $@

$(BUILD)/ld.sw.so: $(LIBC_BASE)/elf/ld.so $(STATIC_REWRITER)
	$(STATIC_REWRITER_DIR)/build/static_rewriter \
		--keep_new_phdrs_vaddr_aligned true \
		--input $(filter %.so,$^) --output $@

$(BUILD)/ld_bss.sw.so: $(LIBC_BASE)/elf/ld.so $(STATIC_REWRITER) $(LD_BSS)
	$(STATIC_REWRITER_DIR)/build/static_rewriter \
		--keep_new_phdrs_vaddr_aligned true \
		--ignore-func strrchr \
		--reloc-end-file $(MAKE_BSS_REAL_DIR)/build/reloc_end \
		--input $< --output $@

$(BUILD)/libdl.sw.so: $(LIBC_BASE)/dlfcn/libdl.so

$(BUILD)/librt.sw.so: $(LIBC_BASE)/rt/librt.so

$(BUILD)/libpthread.sw.so: $(LIBC_BASE)/nptl/libpthread.so

%.sw.so: $(STATIC_REWRITER)
	$(STATIC_REWRITER_DIR)/build/static_rewriter \
		--input $(filter %.so,$^) --output $@
