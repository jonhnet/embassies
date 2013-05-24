#!/usr/bin/python
import sys
pid = int(sys.argv[1])
fp = open("/proc/%d/stat" % pid, "r")
line = fp.read()
fields = line.split()
vsize = int(fields[22])
rss = int(fields[23])
print "vsize %dM rss %dM\n" % (vsize>>20, rss>>(20-12))
