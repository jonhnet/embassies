#!/bin/bash
# This script installs the linux_dbg profiler into midori, for doing
# straight-across sampling-profiler comparisons.
#
# (I used it while trying to diagnose the JavaScript perf difference.
# It turned out not to be the critical tool, but I'll leave this here
# in case it's helpful.)
#
# (TODO I wonder if I could build a generic version in a shared library that
# injects itself with the dynamic loader. Hmm... Maybe even without any
# code at all in the app, so I don't have to touch the app? That'd be keen.)
#
# Anyway, to install, add to midori/main.c:
# #include "LiteLib.h"
# #include "profiler.h"
# #include "standard_malloc_factory.h"
# and
# 	Profiler profiler;
# 	profiler_init(&profiler, standard_malloc_factory_init());
#

cd build/source-files/midori-0*/midori
ln -sf ../../../../../../../../ zoog_root
for i in \
	toolchains/linux_elf/common/bogus_ebp_stack_sentinel.h \
	toolchains/linux_elf/common/invoke_debugger.h \
	common/utils/xax_util.h \
	common/utils/xax_util.c \
	common/utils/c_qsort.h \
	common/utils/c_qsort.c \
	common/utils/cheesylock.h \
	common/utils/cheesylock.c \
	common/utils/cheesy_snprintf.h \
	common/utils/cheesy_snprintf.c \
	common/utils/hash_table.h \
	common/utils/hash_table.c \
	common/utils/LiteAssert.h \
	common/utils/LiteLib.h \
	common/utils/LiteLib.c \
	common/utils/malloc_factory.h \
	common/utils/standard_malloc_factory.h \
	common/utils/standard_malloc_factory.c \
	common/utils/math_util.h \
	common/ifc/pal_abi/ \
	monitors/linux_dbg/pal/MTAllocation.h \
	monitors/linux_dbg/pal/MTAllocation.c \
	monitors/linux_dbg/pal/profiler.h \
	monitors/linux_dbg/pal/profiler.c \
	monitors/linux_dbg/pal/profiler_ifc.h \
	monitors/linux_dbg/pal/posix_profiler_ifc.c \
	; do
ln -sf zoog_root/$i .
done
