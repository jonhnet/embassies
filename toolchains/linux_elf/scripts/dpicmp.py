#!/usr/bin/python

# A tool to come up with some elegant analysis of what libraries
# compose various dpis

import subprocess
import re
import os
import sys

here = os.path.dirname(sys.argv[0])
BINDIR = here+"/../lib_links/"
LIBRARY_PATH=("LD_LIBRARY_PATH="
	+here+"/../apps/gnucash/build/source-files/gnucash-2.2.9/debian/gnucash/usr/lib/gnucash/gnucash/"
	":"+here+"/../apps/gnucash/build/source-files/gnucash-2.2.9/debian/gnucash/"
	":"+here+"/../apps/gnucash/build/source-files/gnucash-2.2.9/debian/gnucash/usr/lib/gnucash/"
	)

class Lib:
	def __init__(self, refname, resolution, line, app):
		self.refname = refname
		self.resolution = resolution
		try:
			self.size = os.stat(self.resolution).st_size
		except:
			self.size = 0
			if (line.startswith("linux-gate")):
				pass
			else:
				print "can't open %s in '%s' from %s" % (self.resolution, line, app)

	def __hash__(self):
		return hash(self.refname)

	def __cmp__(self, other):
		return cmp(self.refname, other.refname)

	def __repr__(self):
		return self.refname

class Ldd:
	def __init__(self, file):
		self.file = file
		self.path = BINDIR + file;
		self.libs = set()
		cmd = "%s ldd %s" % (LIBRARY_PATH, self.path)
#		print cmd
		fp = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		for line in fp.stdout:
			line = line.strip()
			refname = line.split()[0]
			try:
				resolution = line.split()[2]
			except IndexError:
				resolution = ""
			mo = re.compile("^(.*).so.*$").match(refname)
			refname = mo.groups(1)[0]
			refname = refname.split("/")[-1]
			if (line.startswith("/lib/ld-linux")):
				resolution = "/lib/ld-linux.so.2"
			self.libs.add(Lib(refname, resolution, line, file))
#		print "%s: %d libs" % (self.file, len(self.libs))

class ByLib:
	def __init__(self, ldds):
		self.ldds = ldds
		self.libmap = {}
		self.load(ldds)
		self.collate()

	def load(self, ldds):
		for ldd in ldds:
			for lib in ldd.libs:
				self.incr(lib)

	def incr(self, lib):
		if (lib not in self.libmap):
			self.libmap[lib] = 0
		self.libmap[lib] += 1

	def collate(self):
		def make_mask(lib):
			mask = []
			maskv = 0
			for ldd in self.ldds:
				if (lib in ldd.libs):
					mask.append(1)
					maskv = maskv*2 + 1
				else:
					mask.append(0)
					maskv = maskv*2 + 0
			return (maskv,mask)

		for (key,value) in self.libmap.items():
			key.count = value
			key.mask = make_mask(key)

		keys = self.libmap.keys()
		def by_count_then_mask(a, b):
			v1 = -cmp(a.count, b.count)
			if (v1!=0): return v1
			return -cmp(a.mask, b.mask)
		def by_count_then_size(a, b):
			v1 = -cmp(a.count, b.count)
			if (v1!=0): return v1
			return -cmp(a.size, b.size)
		keys.sort(cmp = by_count_then_mask)
#		for key in keys:
#			print "%s: count %d size %d" % (key,key.count,key.size)
		headers=",".join(map(lambda ldd: ldd.file, self.ldds))
		print headers
		for lib in keys:
			mask = lib.mask[1]
			print "%s,%s,%s" % (
				",".join(map(str, mask)),
				lib,
				lib.size
				)
				#+ " %s"%lib.mask[0]
#			v = []
#			for ldd in self.ldds:
#				if (lib in ldd.libs):
#					v.append("X")
#				else:
#					v.append(" ")
#			print ",".join(v)

def main():
	apps =  [
		"midori-pie",
		"gnumeric-pie",
		"abiword-pie",
		"gnucash-pie",
		"gimp-pie",
		"inkscape-pie",
		"marble-pie",
		]
	ldds = map(Ldd,apps);
	def by_size(a,b):
		return -cmp(len(a.libs), len(b.libs))
	ldds.sort(by_size)
	ByLib(ldds);

main()
