#!/usr/bin/python
#
# A script to scan a zarfile and determine how much of the file
# we could remove by stripping symbols from ELF files.
#

import os
import stat
import sys

def file_size(path):
	s = os.stat(path)
	return s[stat.ST_SIZE]
	
def examine(zarfile, path):
	tmp = "/tmp/zarfile_examine.tmp"
	try:
		os.unlink(tmp)
	except OSError:
		pass
	os.system("build/zarfile_tool --extract %s %s > %s" % (zarfile, path, tmp))
	file_spec = os.popen("file -b %s" % tmp)
	result = file_spec.readline().strip()
	file_spec.close()
	#print path, result
	orig_size = file_size(tmp)
	stripped_size = orig_size
	if (result.startswith("ELF") and result.endswith("not stripped")):
		os.system("strip %s" % tmp)
		stripped_size = file_size(tmp)
		savings = orig_size - stripped_size
		print "save %d bytes on %s" % (savings, path)
	return (orig_size, stripped_size)

def enumerate(zarfile):
	file_list = os.popen("build/zarfile_tool --list %s" % zarfile)
	orig_sizes,stripped_sizes = (0,0)
	while (True):
		line = file_list.readline()
		if (line==""):
			break;
		line = line.strip()
		url = line.split()[-1]
		if (url.startswith("file:")):
			path = url[5:]
			#print path
			(orig_size, stripped_size) = examine(zarfile, url)
			orig_sizes += orig_size
			stripped_sizes += stripped_size
	savings = orig_sizes - stripped_sizes
	print "could save %d bytes (%.0f%%)" % (savings, savings*100.0/orig_sizes)

def main():
	try:
		zarfile = sys.argv[1]
	except:
		sys.stderr.write("Usage: %s <zarfile>\n" % sys.argv[0])
		sys.exit(1)
	enumerate(zarfile)

main()
