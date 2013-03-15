ZOOG_ROOT=../../../..

PACKAGE_NAME=abiword
PIE_NAME=abiword-pie

DISTRO=squeeze
ifeq ($(DISTRO), squeeze)

SRCDIR=build/source-files/abiword-*/src
POST_PATCH_SCRIPT=\
	patch $(SRCDIR)/Makefile < squeeze.Makefile.patch && \
	(cd $(SRCDIR) && \
		rm abiword && \
		make abiword )
BUILT_PIE=$(SRCDIR)/.libs/abiword

else ifeq ($(DISTRO), lenny)

SRCDIR=build/source-files/abiword-*/build/src
APPDIR=$(SRCDIR)/wp/main/unix
POST_PATCH_SCRIPT=\
	patch $(APPDIR)/GNUmakefile < lenny.GNUmakefile.patch && \
	(cd $(APPDIR) && \
		rm abiword && \
		make abiword )
BUILT_PIE=$(APPDIR)/abiword

endif

TARGETS=$(PIE_NAME)

NEW_CFLAGS="-g"
NEW_LDFLAGS=""

include ../make.in
