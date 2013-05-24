#!/usr/bin/python

class Counter:
	def __init__(self):
		self.count = 0
		self.bytes = 0

	def __repr__(self):
		return "%d files, %d MiB" % (self.count, self.bytes>>20)

types = {}
fp = open("debug_mmap")
for line in fp:
	line = line.strip()
	fields = line.split()
	if (fields[0]=="M"):
		start = int(fields[2], 16)
		end = int(fields[3], 16)
		bytes = end-start
		disp = fields[5]
		if (disp not in types):
			types[disp] = Counter()
		types[disp].count += 1
		types[disp].bytes += bytes
		print disp
print types
