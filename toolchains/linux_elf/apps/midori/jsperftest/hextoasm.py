#!/usr/bin/python
import sys
import struct

def main(infile):
	ifp = open(infile, "r")
	base = infile.split(".")[0]
	ofp = open("%s.S"%base, "w")
	
	ofp.write("%s:\n" % base)
	for line in ifp:
		if (line==""): break
		line = line.strip()
		while (len(line)>0):
			piece = line[0:2]
			line = line[2:]
			assert(len(piece)==2)
			val = int(piece,16)
			ofp.write("\t.byte %d\n" % val)
	ifp.close()
	ofp.close()

main(sys.argv[1])
