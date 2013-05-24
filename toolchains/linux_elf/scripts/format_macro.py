#!/usr/bin/python
import sys
import re

def strip_trailer(s):
	i = len(s)
	while (i>0 and (s[i-1]=='\\' or s[i-1]==' ')):
		i-=1;
	return s[0:i]

def main():
	while (1):
		line = sys.stdin.readline()
		if (line==""): break
		line = line.rstrip()
		line = line.expandtabs(4)
		line = strip_trailer(line);
		line = "%-77s\\\n" % line
		sys.stdout.write(line)

main()
