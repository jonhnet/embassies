#!/usr/bin/python

import sys

for i in range(int(sys.argv[1])):
	sys.stdin.readline()

while (True):
	l = sys.stdin.readline()
	if (l==""):
		break
	sys.stdout.write(l)
