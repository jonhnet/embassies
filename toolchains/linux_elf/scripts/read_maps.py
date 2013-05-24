#!/usr/bin/python
import sys

class Mapping:
	def __init__(self, start, end, filename, inode, prot, text):
		self.start = start
		self.end = end
		self.filename = filename
		self.inode = inode
		self.prot = prot
		self.text = text

	def size(self):
		return self.end - self.start

class Maps:
	def __init__(self):
		self.maps = []

	def load(self, pid):
		fp = open("/proc/%d/maps" % pid, "r")
		while (True):
			text = fp.readline()
			if (text==""): break
			text = text.strip()
			start = int(text[0:8],16)
			end = int(text[9:17],16)
			prot = text[18:22]
			inode = text[32:37]
			filename = text[49:],16
			mapping = Mapping(start, end, filename, inode, prot, text)
			self.maps.append(mapping)

	def filter(self, filter_func=None):
		if (filter_func==None):
			filter_func = lambda x: True
		m = Maps()
		m.maps = filter(filter_func, self.maps)
		return m

	def size(self):
		total = 0
		for mapping in self.maps:
			total += mapping.size()
		return total

def f_accessible(mapping):
	return not mapping.prot.startswith("---")

def f_disk(mapping):
	return f_accessible(mapping) and mapping.inode[:3]=="fe:"

def f_mem(mapping):
	return f_accessible(mapping) and mapping.inode=="00:00"

def f_shm(mapping):
	return f_accessible(mapping) and mapping.inode=="00:04"

def f_inaccessible(mapping):
	return not f_accessible(mapping)

def f_rest(mapping):
	return not f_disk(mapping) and not f_mem(mapping) and not f_shm(mapping) and not f_inaccessible(mapping)

def by_size_descending(m0, m1):
	return int(m1.size() - m0.size())

def main():
	pid = int(sys.argv[1])
	maps = Maps()
	maps.load(pid)
	print "Total mapped size %dM" % (maps.size()>>20)

	categories = [
		("disk", f_disk),
		("mem", f_mem),
		("shm", f_shm),
		("inaccessible", f_inaccessible),
		("rest", f_rest),
		]
	for (f_name,f_func) in categories:
		group = maps.filter(f_func)
		print "%s size %dM" % (f_name, group.size()>>20)
		maplist = group.maps
		maplist.sort(by_size_descending)
		for mapping in maplist:
			print "  %dM %s" % (mapping.size()>>20, mapping.filename)
			print "    %s" % mapping.prot

main()
