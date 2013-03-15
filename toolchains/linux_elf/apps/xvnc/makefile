ZOOG_ROOT=../../../..

PACKAGE_NAME=vnc4server
PIE_NAME=xvnc-pie

CACHED_SOURCE_TARBALLS=xvnc

PRE_PATCH_SCRIPT=\
	sed -i 's/DefaultGcc2i386Opt -O2/DefaultGcc2i386Opt -pie -g/' build/source-files/*/unix/xc/config/cf/xfree86.cf && \
	(cd build/source-files/*/common/rfb/; echo ZOOG_ROOT=$(abspath $(ZOOG_ROOT)) > Makefile.zoog) && \
	cat src/rfb/*.patch | patch -p0
#	sed 's%ZOOG_ROOT%'$(abspath $(ZOOG_ROOT))'%g' < rfb-Makefile.in.patch | patch build/source-files/*/common/rfb/Makefile.in && \
#	cp src/rfb/* build/source-files/*/common/rfb

# POST_PATCH_SCRIPT=\
# 	sed -i 's/EXTRA_LIBRARIES =/EXTRA_LIBRARIES = -pie/' build/source-files/*/unix/xc/programs/Xserver/Makefile && \
# 	(cd build/source-files/*/unix/xc; make -f xmakefile -w -k World > ../../../build-2.log 2>&1)

BUILT_PIE=build/source-files/*/unix/xc/programs/Xserver/Xvnc

include ../make.in
