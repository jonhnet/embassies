#!/bin/bash

(cd build/source-files/midori-0.2.4 &&
	env WAFDIR=debian ./waf --nocache build --debug full &&
	cp _build_/default/midori/midori debian/midori/usr/bin/midori) &&
rm build/stamp-post-patched &&
make

