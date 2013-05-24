#!/usr/bin/python
#
# read a strace file, or debug_strace file, and produce a table of
# call types and counts.

import sys
import re

table = {}
for line in sys.stdin:
	line = line.strip()
	mo = re.compile(" ([^(]*)\(").search(line)
	if (mo!=None):
		syscall = mo.groups()[0]
		if (syscall not in table):
			table[syscall] = 0
		table[syscall] += 1

pairs = map(lambda i: (i[1],i[0]), table.items())
pairs.sort()
for (v,k) in pairs:
	print "%-30s: %s" % (k,v)
