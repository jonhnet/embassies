ZOOG_ROOT=../../../..

PACKAGE_NAME=vnc4server
PIE_NAME=xvnc-pie

CACHED_SOURCE_TARBALLS=xvnc

PRE_PATCH_SCRIPT=\
	sed -i 's/DefaultGcc2i386Opt -O2/DefaultGcc2i386Opt -pie -g/' build/source-files/*/unix/xc/config/cf/xfree86.cf

BUILT_PIE=build/source-files/*/unix/xc/programs/Xserver/Xvnc

include ../make.in
