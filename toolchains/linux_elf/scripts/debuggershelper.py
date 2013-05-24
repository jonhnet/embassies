#!/usr/bin/python

import inspect
import re
import os
import sys
import struct
import stat
import signed_binary_magic

cache = {}

def find_zoog_root():
	mypath = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))+"/../../.."
	zoog_root = os.path.abspath(mypath)+"/"
	return zoog_root

zoogroot = find_zoog_root()
g_find_zoog_note = zoogroot+"toolchains/linux_elf/scripts/build/find_zoog_note"
g_elfLoaderDir = os.getcwd()+"/"
g_corefile_default_elf_loader = zoogroot+"zoog/toolchains/linux_elf/elf_loader/build/elf_loader_zguest.raw"
g_elfLoaderDLPath = zoogroot+"toolchains/linux_elf/libc/build/ld.sw.so"

def get_file_len(pal_path):
	sr = os.stat(pal_path)
	return sr[stat.ST_SIZE]

class NotELFException(Exception):
	def __init__(self, m):
		Exception.__init__(self, m)

class NoTextStartException(Exception):
	def __init__(self, m):
		Exception.__init__(self, m)

class FuncRec:
	def __init__(self, name, addr, size):
		self.name = name
		self.addr = addr
		self.size = size

	def __cmp__(self, other):
		if (isinstance(other, FuncRec)):
			other_addr = other.addr
		else:
			other_addr = other
		return cmp(self.addr, other_addr)

	def __repr__(self):
		return "%s:%08x--%08x" % (self.name, self.addr, self.addr+self.size)

class FuncTable:
	def __init__(self, elf_file_path):
		fp = os.popen("readelf --syms '%s' | grep FUNC | sort -k2" % elf_file_path, "r")
		self.table = []
		while (1):
			line = fp.readline()
			if (line==""): break
			line = line.strip()
			line = line[8:]
			fields = re.split(' *', line)
			name = fields[6]
			addr = int(fields[0],16)
			size = int(fields[1])
			_type = fields[2]
			if (_type != "FUNC"):
				continue
			fr = FuncRec(name, addr, size)
			self.table.append(fr)
		fp.close()

	def find(self, addr):
		idx = self.find_index(addr, 0, len(self.table)-1)
		if (idx >= 0 and idx < len(self.table)):
			return self.table[idx]
		else:
			return None

	def find_index(self, addr, lo, hi):
		# print "search %d..%d" % (lo,hi)
		if (lo>=hi):
			return hi
		split = (lo+hi)/2
		# print "  split %s" % self.table[split]
		if (addr == self.table[split]):
			return split
		elif (addr < self.table[split]):
			return self.find_index(addr, lo, split)
		else:
			return self.find_index(addr, split+1, hi)

def section_shape(path, section):
	if (path in cache):
		return cache[(path,section)]
	
	try:
		fd = open(path, "r")
	except IOError:
		raise NotELFException(path)
	magic = fd.read(4)
	fd.close()

	if (magic != "\x7fELF"):
		raise NotELFException(path)

	fp = os.popen("readelf --sections %s" % path, "r")
	addr = None
	size = None
	while (1):
		line = fp.readline()
		if (line==""): break
		line = line.strip()
		line = line[5:]
		fields = re.split(' *', line)
		try:
			if (fields[0]==section):
				addr = int(fields[2], 16)
				size = int(fields[4], 16)
				break
		except:
			pass
	fp.close()
	cache[(path,section)] = (addr, size)
	if (addr==None or size==None):
		raise NoTextStartException("No textstart in %s; missing section %s" % (path, section))
	return (addr,size)

def lookupSymbol(file, symbol):
	cmd = "readelf --symbols %s | grep 'OBJECT.*%s'" % (file, symbol)
	fp = os.popen(cmd, "r")
	lines = fp.read().split("\n")[:-1]
	if (len(lines)==0):
		return None
	fp.close()

	#print "unfiltered lines", lines
	lines = filter(lambda l: l.split(' ')[-1]==symbol, lines)
	def rangeForLine(line):
		fields = re.compile(" *").split(line)
		loaderAddr = int(fields[2], 16)
		loaderLen = int(fields[3], 16)
		return Range(loaderAddr, loaderAddr+loaderLen, 0)
	ranges = map(rangeForLine, lines)

	for extraRange in ranges[1:]:
		if (extraRange.cump(ranges[0])!=0):
			raise Exception("Multiple matches for symbol '%s'?? %s" % (symbol, lines))

	#print "lookup(%s) = %s" % (symbol, ranges[0])
	return ranges[0]

# These constants are derived from linux_elf/elf_flatten/elf_flatten.c's
# struct RawPrefix.
raw_prefix_jump_region = 0x40
raw_prefix_dbg_elf_path = 0x100
raw_elf_offset = raw_prefix_jump_region + raw_prefix_dbg_elf_path

def findElfLoaderDLOffset(sbpath):
	fp = open(sbpath)
	fp.seek(0);
	# TODO barf 4s and 8s encoding signed_binary struct size.
	sb_magic = struct.unpack("!I", fp.read(4))[0];
	assert(sb_magic==signed_binary_magic.big_endian_magic)
	cert_size = struct.unpack("!I", fp.read(4))[0];
	raw_offset = signed_binary_magic.struct_size+cert_size
	fp.seek(raw_offset+raw_prefix_jump_region)
	elf_path = fp.read(raw_prefix_dbg_elf_path)
	elf_path = elf_path.replace(chr(0), '')

	# assert here that our offsets are consistent
	# with definition in elfFlatten.
	fp.seek(raw_offset+raw_elf_offset)
	elf_magic = fp.read(4)
	assert(
		ord(elf_magic[0])==127
		and elf_magic[1:4]=="ELF")

	fp.close()

	range = lookupSymbol(elf_path, "loader")
	if (range==None):
		#sys.stderr.write("couldn't find loader in %s\n" % elf_path)
		return (None, elf_path)
#	base = findElfLoaderBaseAddr(elfLoaderPath)
#	range = range.offset(-base)
	range = range.offset(raw_elf_offset)

	return (range, elf_path)

def findElfLoaderBaseAddr(elfLoaderPath):
	fp = os.popen("readelf --program-headers %s | grep 'Entry'" % elfLoaderPath, "r")
	line = fp.readline()
	fp.close()
	elfBaseAddr = int(line.split(' ')[2],16)
	return elfBaseAddr

class Range:
	def __init__(self, start, end, file_offset):
		self.start = start
		self.end = end
		self.file_offset = file_offset

	def contains(self, value):
		return (value >= self.start and value < self.end)

	def offset(self, value):
		return Range(self.start+value, self.end+value, self.file_offset)

	def __repr__(self):
		return "R[0x%08x:0x%08x @%06x]" % (
			self.start, self.end, self.file_offset)

	def tuple(self):
		return (self.start, self.end, self.file_offset)

	# hmm, this as __cmp__ breaks something else. Not sure why.
	def cump(self, other):
		if (other==None):
			return False
		return cmp(self.tuple(), other.tuple())

class FileMap:
	def __init__(self, name, ignore_textstart = False):
		self.name = name
		self.mappings = []
		assert(not ignore_textstart)	# deprecated
		self.ignore_textstart = ignore_textstart
#		self.memoized_textstart_val = None
#		self.memoized_text_section_start = None
		self.memoized_sections = {}
		self.memoized_func_table = None

	def get_func_table(self):
		if (self.memoized_func_table == None):
			self.memoized_func_table = FuncTable(self.name)
		return self.memoized_func_table

	def addMapping(self, range):
		self.mappings.append(range)

	def getMainMapping(self):
		if (len(self.mappings)==1):
			return self.mappings[-1]
		elif (len(self.mappings)==0):
			#print "Crud! No mappings! for %s" % self.name
			return None
		else:
			# Some mappers (eg. zguest) mmap a file to inspect it,
			# then mmap it into its final destination.
			# That first mapping isn't the one we want.
			# So we select all the mappings that start at the beginning
			# of the file (because that's where the .text segment always
			# seems to live), and then take the most-recent such mapping.
			zero_offset_mappings = filter(lambda m: m.file_offset==0, self.mappings)
			return zero_offset_mappings[-1]

	def section_start(self, section_name):
		if (section_name not in self.memoized_sections):
			self.memoized_sections[section_name] = (
				section_shape(self.name, section_name)[0])
		return self.memoized_sections[section_name]

	def adjusted_section_start(self, section_name):
#		if (self.memoized_textstart_val != None):
#			return self.memoized_textstart_val
		mainMapping = self.getMainMapping()
		if (mainMapping==None):
			return ""
		start_addr = mainMapping.start
		if (self.ignore_textstart):
			result = start_addr
		else:
			try:
				result = self.section_start(section_name)
			except NotELFException, ex:
				return ""
			except NoTextStartException, ex:
				print "Warning: %s" % ex
				return None
			if (result < 0x1000000):
				result += start_addr
		return result
		#print "name %s textstart %08x start_addr %08x\n" % (self.name, result, start_addr)
#		self.memoized_textstart_val = result
#		return self.memoized_textstart_val

	def emit(self):
		extra_params = "";
		for extra_section in [".bss", ".data"]:
			section_offset = self.adjusted_section_start(extra_section)
			if (section_offset == None):
				continue
			extra_params += " -s %s 0x%08x" % (extra_section, section_offset)
		textstart = self.adjusted_section_start(".text")
		if (textstart != None):
			return "add-symbol-file %s 0x%08x %s\n" % (self.name, textstart, extra_params)
		else:
			# this can happen with data-only libraries
			return ""

	def __repr__(self):
		return "%s(%s mappings)" % (self.name, len(self.mappings))

	def __len__(self):
		return len(self.mappings)

	def is_elf(self):
		try:
			fp = open(self.name)
		except IOError, ex:
			if (self.name.endswith(".so")):
				sys.stderr.write("warning: ignoring %s\n" % self.name)
			return False
		prefix = fp.read(4)
		fp.close()
		return (prefix=="\177ELF")

	def find_symbol(self, file_eip, dbg_orig_eip):
		#print "Searching for %08x (orig %08x)" % (file_eip, dbg_orig_eip)
		ft = self.get_func_table()
		fr = ft.find(file_eip)
		#print "Found FuncRec %s" % fr
		return fr

	def lookup_func(self, eip):
		mm = self.getMainMapping()
		if (mm.contains(eip)):
			rel_addr = eip - self.adjusted_section_start(".text") + self.section_start(".text")
			sym = self.find_symbol(rel_addr, eip)
			return (sym, rel_addr)
		else:
		# TODO annoying bug: range computation is incorrect and lops the
		# tail off of files, so e.g. SHA256 is not found in elf_loader.elf.
#			rel_addr = eip - self.adjusted_section_start() + self.text_section_start()
#			print "eip %08x @%x not in %s %s (start %x end %x)" % (eip, rel_addr, self.name, mm, mm.start, mm.end)
			return (None, None)

def raw_to_elf(path):
	path = path.replace("_zguest.raw", ".elf")
	path = path.replace(".raw", ".elf")
	return path

class Scan:
	def __init__(self, corefile):
		self.files = {}

		mappings = []
		if (corefile!=None):
			mappings += self.scan_core_file(corefile)

		mappings += self.scan_debug_mmap()

		mappings = filter(lambda x: len(x)>0, mappings)
		mappings = filter(lambda x: x.is_elf(), mappings)
		self.final_mappings = mappings
		self.emit_addsyms()

	def found_signed_binary(self, addr, sbpath):
		(eldRange, elf_path) = findElfLoaderDLOffset(sbpath)
		sys.stderr.write("found_signed_binary elf %s raw %s\n" % (elf_path, sbpath));

		#print "addr: %08x eldlo %08x\n" % (addr, eldlo)

		mappinglist = []

		if (eldRange!=None):
			# there's an embedded loader we can add symbols for, too.
			eldRange = eldRange.offset(addr)
			#print "offset eldRange %s" % eldRange
			fm = FileMap(g_elfLoaderDLPath)
			fm.addMapping(eldRange)
			mappinglist.append(fm)

		fm0 = FileMap(elf_path)
		size = section_shape(elf_path, ".text")[1]
		el_range = Range(addr, addr+size, 0)
		el_range = el_range.offset(raw_elf_offset)
		fm0.addMapping(el_range)
		mappinglist.append(fm0)
		return mappinglist

	def scan_core_file(self, corefile):
		fp = os.popen(g_find_zoog_note+" %s" % corefile, "r")
		bootblock_addr_s = fp.readline()
		bootblock_addr_s = bootblock_addr_s.strip()
		try:
			bootblock_addr = int(bootblock_addr_s, 16)
		except ValueError:
			print "Error decoding \"%s\"" % bootblock_addr_s
			raise
		bootblock_path = fp.readline()
		bootblock_path = bootblock_path.strip()
		sys.stderr.write("boot block '%s' at addr %08x\n" % (bootblock_path, bootblock_addr));
		return self.found_signed_binary(bootblock_addr, bootblock_path)

	def scan_debug_mmap(self):
		blocks = []
		self.mappinglist = []
		try:
			fp = open("debug_mmap")
		except IOError:
			sys.stderr.write("Warning: no debug_mmap present.\n")
			return []
	
		def reset():
			blocks.append(self.mappinglist)
			self.mappinglist = []

		while (1):
			line = fp.readline()
			if (line==""): break
			line = line.strip()
			fields = line.split(' ')

			if (fields[0]=="R"):
				# a loader ran. But several might. Ignore; use L instead.
				continue

			if (fields[0]=="P"):
				# zoog_kvm_pal ran, reporting the pal itself.
				# (root loader reported in core file fields)
				reset()
				pal_path = fields[1]
				addr = int(fields[2], 16)
				fm = FileMap(pal_path)
				self.mappinglist.append(fm)
				fm.addMapping(Range(addr, addr+get_file_len(pal_path), 0))
				continue
				
			if (fields[0]=="L"):
				# xaxportpal ran, reporting the root loader.
				reset()
				raw_rel_path = fields[1]
				raw_path = os.path.join(g_elfLoaderDir, raw_rel_path)
				addr = int(fields[2], 16)
				self.mappinglist += self.found_signed_binary(addr, raw_path)

				continue

			if (fields[0]=="F"):
				fid = int(fields[1], 16)
				name = fields[2]
				if (name.startswith("/xax-libraries")):
					name = name[14:]
				fm = FileMap(name)
				self.files[fid] = fm
				self.mappinglist.append(fm)
				continue

			if (fields[0]=="M"):
				fid = int(fields[1], 16)
				start_addr = int(fields[2], 16)
				end_addr = int(fields[3], 16)
				offset = int(fields[4], 16)
				self.files[fid].addMapping(Range(start_addr, end_addr, offset))
				continue

		blocks.append(self.mappinglist)
		fp.close()
		return blocks[-1]

	def emit_addsyms(self):
		ofp = open("addsyms", "w")
		for mapping in self.final_mappings:
			ofp.write(mapping.emit()+"")
		ofp.close()

	def lookupsym(self, symname, eip):
		for fm in self.final_mappings:
			mm = fm.getMainMapping()
			if (mm!=None and mm.contains(eip)):
				range = lookupSymbol(fm.name, symname)
				if (range==None):
					print "No symbols in %s?" % fm.name
					continue
				print "symbol %s per eip 0x%08x appears at 0x%08x in %s" % (symname, eip, range.start+mm.start, fm.name)
				#print " mapping at    0x%08x" % mm.start
				#print " symbol at     0x%08x" % range.start

	def lookup_func(self, eip):
		for fm in self.final_mappings:
			(sym,rel_addr) = fm.lookup_func(eip)
			if (sym!=None):
				return (sym,rel_addr)
		return (None,None)

	def substitute_profiler_syms(self, fn):
		fp = open(fn)
		contents = fp.read()
		fp.close()

		def repl(field):
			eip = int(field.group(1), 16)
			(sym,rel_addr) = self.lookup_func(eip)
			try:
				name = sym.name
				rel_addr = "+%x" % rel_addr
			except:
				name = "None"
				rel_addr = ""
			return "0x%08x %s %s" % (eip, name, rel_addr)
		new_contents = re.sub("EIP\(([^)]*)\)", repl, contents)
		print new_contents

class SymLookup:
	def __init__(self, symname, eip):
		self.symname = symname
		self.eip = eip

class Args:
	def __init__(self, argv):
		self.symLookup = None
		self.corefile = None
		self.profiler_file = None
		while (len(argv)>0):
			if (argv[0]=="--sym"):
				self.symLookup = SymLookup(argv[1], int(argv[2], 16))
				argv = argv[3:]
			elif (argv[0]=="--core"):
				self.corefile = argv[1]
				argv = argv[2:]
			elif (argv[0]=="--profiler"):
				self.profiler_file = argv[1]
				argv = argv[2:]
			else:
				raise Exception("usage: [--sym <symname> <addr>|--core corefile]")

def main():
	args = Args(sys.argv[1:])

	scan = Scan(args.corefile)

	#print "\n".join(map(str,scan.final_mappings))

	if (args.symLookup!=None):
		#symname = sys.argv[2]
		scan.lookupsym(args.symLookup.symname, args.symLookup.eip)

	if (args.profiler_file!=None):
		scan.substitute_profiler_syms(args.profiler_file)

main()
