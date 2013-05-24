#!/usr/bin/python

import re

INVALID_FD = 0xffffffff

class Mapping:
	def __init__(self, addr, length, offset):
		self.addr = addr
		self.length = length
		self.offset = offset

	def __cmp__(self, other):
		return cmp(self.offset, other.offset)

class StateMachine:
	def __init__(self, path):
		self.path = path
		self.is_so = ".so" in self.path
		self.state = "read-phase"
		self.mmaps = []

	def read(self):
		if (self.state != "read-phase"):
			self.dead()

	def seek(self):
		if (self.state != "read-phase"):
			self.dead()

	def mmap(self, mapping):
		self.mmaps.append(mapping)
		if (self.state == "read-phase"):
			self.state = "mmap-phase"
		elif (self.state != "mmap-phase"):
			self.dead()
	
	def write(self):
		self.dead()

	def dead(self):
		if (self.state != "broken"):
			if (self.is_so):
				print "Path %s dies" % self.path
		self.state = "broken"

	def happy(self):
		return self.state != "broken"

	def get_mmaps(self):
		self.mmaps.sort()
		return self.mmaps
		
	def remark_mmap(self):
		self.mmaps.sort()
		print self.path
		base = self.mmaps[0]
		for mmap in self.mmaps:
			print "  A%8x .. A+%8x: L%8x O%8x (+%8x)" % (
				mmap.addr,
				mmap.addr-base.addr,
				mmap.length,
				mmap.offset,
				mmap.offset - (mmap.addr-base.addr))

class MmapAnalysis:
	def __init__(self):
		self.opened_files = {}
		self.so_sms = []
		self.opened_files[0] = self.makeStateMachine("stdin")
		self.opened_files[1] = self.makeStateMachine("stdout")
		self.opened_files[2] = self.makeStateMachine("stderr")
		self.readfile()
		self.remark_sms()

	def makeStateMachine(self, path):
		sm = StateMachine(path)
		self.so_sms.append(sm)
		return sm

	def dequote(self, s):
		if (s[0]=='"'):
			s = s[1:]
		if (s[-1]=='"'):
			s = s[:-1]
		return s

	def readfile(self):
		stracere = re.compile("^THR.*: __NR_(.*)\((.*)\) = (.*)$")

		linenum = 0
		for line in open("debug_strace"):
			linenum += 1
			mo = stracere.match(line)
			if (mo==None): continue
			(syscall,args_str,return_val) = mo.groups()
			self.cur_linenum = linenum
			self.cur_syscall = syscall
			args = args_str.split(", ")
			#print linenum, ":", syscall, args, return_val
			if (syscall=="open"):
				self.open(self.dequote(args[0]), int(return_val, 16))
			elif (syscall=="close"):
				self.close(int(args[0], 16))
			elif (syscall=="mmap2"):
				#addr = int(args[0], 16)
				addr = int(return_val, 16)
				length = int(args[1], 16)
				fd = int(args[4], 16)
				offset = int(args[5], 16)
				self.mmap(addr, length, fd, offset)
			elif (syscall=="read"):
				self.read(int(args[0],16))
			elif (syscall=="write"):
				self.write(int(args[0],16))
			elif (syscall=="lseek"):
				self.seek(int(args[0],16))

	def open(self, path, return_val):
		if (return_val > 0x7fffffff):
			return
		self.opened_files[return_val] = self.makeStateMachine(path)

	def getfile(self, fd):
		if (fd not in self.opened_files):
			self.opened_files[fd] = self.makeStateMachine(
				"first seen at %d:%s" % (self.cur_linenum, self.cur_syscall))
		return self.opened_files[fd]

	def close(self, fd):
		if (fd not in self.opened_files):
			return
		sm = self.opened_files[fd]
		if (sm.is_so and sm.happy()):
			print "%s exits having followed the rules, in %s" % (sm.path, sm.state)
		del self.opened_files[fd]

	def seek(self, fd):
		self.getfile(fd).seek()

	def read(self, fd):
		self.getfile(fd).read()

	def write(self, fd):
		self.getfile(fd).write()

	def mmap(self, addr, length, fd, offset_in_pages):
		if (fd==INVALID_FD):
			return
		self.getfile(fd).mmap(Mapping(addr, length, offset_in_pages<<12))

	def remark_sms(self):
		libmap = {}
		sos = filter(lambda sm: sm.is_so, self.so_sms)
		for sm in sos:
			if (sm.path not in libmap):
				libmap[sm.path] = MapCounter()
			mc = libmap[sm.path]
			#sm.remark_mmap()
			for mmap in sm.get_mmaps():
				if (mmap.offset==0):
					mc.fullmaps_count += 1
					mc.fullmaps_size += mmap.length
				else:
					mc.partial_count += 1
					mc.partial_size = mmap.length	# lazily guessing they all overlap.

		overhead_bytes = 0
		memcmp_reductions = 0
		for (path,mc) in libmap.items():
			print "%s" % path
			print "  full:    %3d copies %3dKB" % (mc.fullmaps_count, mc.fullmaps_size>>10)
			if (mc.fullmaps_count > 1):
				overhead_incr = mc.fullmaps_size*1.0*(mc.fullmaps_count-1)/(mc.fullmaps_count)
				print "    incr %3dKB" % (int(overhead_incr) >> 10)
				overhead_bytes += overhead_incr
			memcmp_reductions += mc.fullmaps_size
			print "  partial: %3d copies %3dKB" % (mc.partial_count, mc.partial_size>>10)
			overhead_bytes += mc.partial_size
		print "memcmp_reductions = %3dMB" % (int(memcmp_reductions)>>20)
		print "overhead_bytes = %3dMB" % (int(overhead_bytes)>>20)

class MapCounter:
	def __init__(self):
		self.fullmaps_count = 0
		self.fullmaps_size = 0
		self.partial_count = 0
		self.partial_size = 0

MmapAnalysis()
