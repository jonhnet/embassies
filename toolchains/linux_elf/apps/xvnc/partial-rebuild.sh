#!/bin/bash
export ZOOG_ROOT=`realpath ../../../..`
XVNC_ROOT=${ZOOG_ROOT}/toolchains/linux_elf/apps/xvnc
cd ${XVNC_ROOT}/build/source-files/vnc4-4.1.1+X4.3.0/common/rfb
rm VNCServerST.o; make
if [ $? != 0 ]; then echo darn; exit -1; fi
cd ${XVNC_ROOT}/build/source-files/vnc4-4.1.1+X4.3.0/unix/xc
make -f xmakefile -w -k World
if [ $? != 0 ]; then echo darn; exit -1; fi
cd ${XVNC_ROOT}
rm build/xvnc-pie; make
