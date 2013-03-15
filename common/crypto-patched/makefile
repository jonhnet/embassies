ZOOG_ROOT=../..
FETCH_CODE=$(ZOOG_ROOT)/toolchains/linux_elf/scripts/fetch-code.py
EXTERNAL_CODE_CACHE=$(ZOOG_ROOT)/external-code
NOCR=sed -i 's/\r//g'

all: patched/vmac.c patched/sha2big.c patched/rijndael.c patched/rijndael.h patched/rsa.cpp

clean:
	rm -rf orig patched

orig/stamp:
	mkdir orig
	touch $@

patched/stamp:
	mkdir patched
	touch $@

##############################################################################

orig/vmac-orig.h: orig/stamp
	$(FETCH_CODE) $@ \
		'http://www.fastcrypto.org/vmac/vmac.h' \
		$(EXTERNAL_CODE_CACHE)/vmac/vmac.h

.PRECIOUS: orig/vmac-orig.h

orig/vmac-orig.c: orig/stamp
	$(FETCH_CODE) $@ \
		'http://www.fastcrypto.org/vmac/vmac.c' \
		$(EXTERNAL_CODE_CACHE)/vmac/vmac.c

.PRECIOUS: orig/vmac-orig.c

patched/vmac.c patched/vmac.h: orig/vmac-orig.h orig/vmac-orig.c patched/stamp
	cp orig/vmac-orig.c patched/vmac.c
	cp orig/vmac-orig.h patched/vmac.h
	patch patched/vmac.c < patches/vmac.c.patch
	patch patched/vmac.h < patches/vmac.h.patch

##############################################################################

orig/rijndael-alg-fst.h: orig/stamp
	$(FETCH_CODE) $@ \
		'http://www.cs.ucdavis.edu/~rogaway/ocb/ocb-ref/rijndael-alg-fst.h' \
		$(EXTERNAL_CODE_CACHE)/rijndael/rijndael-alg-fst.h
	$(NOCR) $@

.PRECIOUS: orig/rijndael-alg-fst.h

orig/rijndael-alg-fst.c: orig/stamp
	$(FETCH_CODE) $@ \
		'http://www.cs.ucdavis.edu/~rogaway/ocb/ocb-ref/rijndael-alg-fst.c' \
		$(EXTERNAL_CODE_CACHE)/rijndael/rijndael-alg-fst.c
	$(NOCR) $@

.PRECIOUS: orig/rijndael-alg-fst.c

patched/rijndael.c: orig/rijndael-alg-fst.c
	sed 's/\r//' < $^ > $@
	patch $@ < patches/rijndael.c.patch

patched/rijndael.h: orig/rijndael-alg-fst.h
	sed 's/\r//' < $^ > $@
	patch $@ < patches/rijndael.h.patch

##############################################################################

# sha2 via:
# http://www.saphir2.com/sphlib/

SHA_VERSION=-3.0

orig/sphlib$(SHA_VERSION).zip: orig/stamp
	$(FETCH_CODE) $@ \
		'http://www.saphir2.com/sphlib/files/sphlib$(SHA_VERSION).zip' \
		$(EXTERNAL_CODE_CACHE)/sphlib/sphlib$(SHA_VERSION).zip

.PRECIOUS: orig/sphlib$(SHA_VERSION).zip

orig/sha2-orig: orig/sphlib$(SHA_VERSION).zip
	rm -rf $@
	mkdir $@
	unzip -d $@ $^

#sha2big.c sha2.h sha2_helper.c.inc sha2small.c sha2_types.h: sha2-orig

patched/sha2big.c: orig/sha2-orig patched/stamp
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sha2big.c patched/sha2big.c
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sha2.c patched/sha2small.c
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sph_sha2.h patched/sha2.h
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sph_types.h patched/sha2_types.h
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/md_helper.c patched/sha2_helper.c.inc
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sph_sha1.h patched/sha1.h
	cp orig/sha2-orig/sphlib$(SHA_VERSION)/c/sha1.c patched/sha1.c
	patch patched/sha2big.c < patches/sha2big.c.patch
	patch patched/sha2small.c < patches/sha2small.c.patch
	patch patched/sha2.h < patches/sha2.h.patch
	patch patched/sha2_types.h < patches/sha2_types.h.patch
	patch patched/sha2_helper.c.inc < patches/sha2_helper.c.inc.patch
	patch patched/sha1.h < patches/sha1.h.patch
	patch patched/sha1.c < patches/sha1.c.patch

##############################################################################

POLARTGZ=polarssl-0.10.0-gpl.tgz

orig/$(POLARTGZ): orig/stamp
	$(FETCH_CODE) $@ \
		'http://polarssl.org/code/releases/$(POLARTGZ)' \
		$(EXTERNAL_CODE_CACHE)/polarssl/$(POLARTGZ)

.PRECIOUS: orig/$(POLARTGZ)

orig/rsa-orig: orig/$(POLARTGZ)
	rm -rf $@
	mkdir $@
	tar -C $@ -xzf $^

patched/rsa.cpp: orig/rsa-orig patched/stamp
	cp patches/rsa_config.h patched/rsa_config.h
	sed 's/\r//' < orig/rsa-orig/polarssl-0.10.0/include/polarssl/rsa.h > patched/rsa.h
	sed 's/\r//' < orig/rsa-orig/polarssl-0.10.0/library/rsa.c > patched/rsa.cpp
	sed 's/\r//' < orig/rsa-orig/polarssl-0.10.0/library/bignum.c > patched/bignum.cpp
	sed 's/\r//' < orig/rsa-orig/polarssl-0.10.0/include/polarssl/bn_mul.h > patched/bignum_mul.h
	sed 's/\r//' < orig/rsa-orig/polarssl-0.10.0/include/polarssl/bignum.h > patched/bignum.h
	patch patched/rsa.h < patches/rsa.h.patch
	patch patched/rsa.cpp < patches/rsa.cpp.patch
	patch patched/bignum.cpp < patches/bignum.cpp.patch
	patch patched/bignum.h < patches/bignum.h.patch
	patch patched/bignum_mul.h < patches/bignum_mul.h.patch
