#!/usr/bin/python

import sys

class MapRec:
	def __init__(self, fd, fn, start, end, offset):
		self.fd=fd
		self.fn=fn
		self.start=start
		self.end=end
		self.offset=offset

def scan(fn, addr):
	empty = MapRec(-1, "Not in a mapped file", 0, 0, 0)
	curfile = empty
	file = {}
	fp = open(fn)
	while (1):
		l = fp.readline()
		if l=="": break
		l = l.strip()
		f = l.split()
		if (f[0]=="L"):
			curfile = empty
		if (f[0]=="F"):
			fd = int(f[1], 16)
			file[fd] = f[2]
		if (f[0]=="M"):
			fd = int(f[1], 16)
			a0 = int(f[2], 16)
			a1 = int(f[3], 16)
			off= int(f[4], 16)
			#print map(lambda s: "%08x"%s, [a0, addr, a1])
			if (a0<=addr and addr<a1):
				curfile = MapRec(fd, file[fd], a0, a1, off)
	if (curfile==empty):
		print "could not find 0x%08x" % addr
	else:
		print "gdb %s" % curfile.fn
		print "base 0x%08x" % (curfile.start)
		print "disass 0x%08x" % (addr - curfile.start + curfile.offset)

def main():
	fn = "debug_mmap"
	sys.argv = sys.argv[1:]
	if (sys.argv[0]=='--mmapfile'):
		fn = sys.argv[1]
		sys.argv = sys.argv[2:]
	addr = int(sys.argv[0], 16)
	scan(fn, addr)

main()
