#!/usr/bin/python

import os

outdir="mmap_check_files"
os.system("rm -rf %s" % outdir)
os.mkdir(outdir);

class Mapping:
	def __init__(self, index, memstart, memend, fileoffset, filename):
		self.index = index
		self.memstart = int(memstart, 16)
		self.memend = int(memend, 16)
		self.fileoffset = int(fileoffset, 16)
		self.filename = filename

	def dbg_cmd(self):
		return "dump memory %s 0x%08x 0x%08x" % (
			self.refname("mem"), self.memstart, self.memend)

	def refname(self, label):
		short = os.path.basename(self.filename)
		return "%s/%02d-%s.%s" % (outdir, self.index, short, label)
		
	def len(self):
		return self.memend-self.memstart

	def extract_sample(self):
		fp = open(self.filename)
		fp.seek(self.fileoffset)
		content = fp.read(self.len())
		fp.close()
		ofp = open(self.refname("ref"), "w")
		ofp.write(content)
		ofp.close()

def get_mappings():
	fp = open("debug_mmap")
	filemap = {}
	mappings = []
	while (1):
		l = fp.readline()
		if (l==""): break
		l = l.strip()
		f = l.split()
		if (f[0]=='F'):
			filemap[f[1]]=f[2]
		elif (f[0]=='M'):
			mappings.append(
				Mapping(len(mappings), f[2], f[3], f[4], filemap[f[1]]))
	return mappings

def main():
	for m in get_mappings():
		print m.dbg_cmd()
		m.extract_sample()

main()
