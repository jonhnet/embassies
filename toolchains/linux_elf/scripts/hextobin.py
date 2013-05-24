#!/usr/bin/python
import sys
import struct
for line in sys.stdin:
	if (line==""): break
	line = line.strip()
	while (len(line)>0):
		piece = line[0:2]
		line = line[2:]
		assert(len(piece)==2)
		val = int(piece,16)
		sys.stdout.write(struct.pack("B", val))
