ZOOG_ROOT=../../../..

PACKAGE_NAME=epiphany-browser
PIE_NAME=epiphany-pie

DISTRO=squeeze
ifeq ($(DISTRO),squeeze)
BUILT_PIE=build/source-files/epiphany-*/src/epiphany
else ifeq ($(DISTRO),lenny)
BUILT_PIE=build/source-files/epiphany-*/debian/build/gecko/src/epiphany
endif

include ../make.in
