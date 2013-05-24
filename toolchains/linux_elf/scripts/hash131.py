#!/usr/bin/python

import sys

fn = sys.argv[1]
fp = open(fn, "rb")
x = fp.read()
fp.close()

v = 0
for xc in x:
	v = (v*131 + ord(xc)) & 0xffffffff
print "%08x %s (0x%x bytes)" % (v, fn, len(x))
