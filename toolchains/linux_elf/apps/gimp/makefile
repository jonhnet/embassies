ZOOG_ROOT=../../../..

PACKAGE_NAME=gimp
PIE_NAME=gimp-pie

DISTRO=lenny
ifeq ($(DISTRO),sarge)
BUILT_PIE=build/source-files/gimp-*/build/app/.libs/gimp-2.6
else ifeq ($(DISTRO),lenny)
BUILT_PIE=build/source-files/gimp-*/debian/gimp/usr/bin/gimp
endif


include ../make.in
