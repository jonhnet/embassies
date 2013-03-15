#!/bin/bash
#
# This assumes you have a directory build/ with your live changes in it,
# and a directory build-reference/ that's built from the stock configuration:
#    make clean; make build/stamp-unpacked; mv build build-reference
#
# It further assumes a short list of directories
# that need scannin'.

path=source-files/webkit-1.2.7
diff -rNu build-reference/${path}/debian/rules build/${path}/debian/rules > zoog_changes.patch
diff -x tags -x navigation_module -x common -x crypto -x ifc -x utils -x zoog_root.h -rNu build-reference/${path}/WebCore build/${path}/WebCore >> zoog_changes.patch
