#!/usr/bin/python

import sys

filename = sys.argv[1]
offset = int(sys.argv[2],16)
length = int(sys.argv[3],16)

ifp = open(filename, "r")
ifp.seek(offset)
bits = ifp.read(length)
sys.stdout.write(bits)
ifp.close()
