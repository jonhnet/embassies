#!/usr/bin/python

import sys

sys.stdout.write("<pre>\n")
seen = {}
while (1):
	l = sys.stdin.readline()
	if l=="":
		break
	o = l
	if (l not in seen):
		o = "<b>"+l+"</b>"
	seen[l] = 1
	sys.stdout.write(o)
sys.stdout.write("</pre>\n")
