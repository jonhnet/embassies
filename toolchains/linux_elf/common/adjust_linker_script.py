#!/usr/bin/python

# Create a linker script, based on the system compiler default,
# that re-bases a binary to the specified address.
#
# Now that we're generally using position-independent executables
# (it's a requirement of the platform),
# we don't much need this technique.
# However, I'm still using it to catch some weird compiler interactions;
# see toolchains/linux_elf/elf_loader/README.environment.

import sys
import os
import re

newaddress = int(sys.argv[1], 16)

fp = os.popen("ld --verbose", "r")
section = 0
script=[]
while (1):
	l = fp.readline()
	if (l==""): break
	l = l[:-1]
	if (l.startswith("=====")):
		section+=1
	elif (section==1):
		script.append(l)

def update(line):
	if (line.find('PROVIDE (__executable_start') >=0 ):
		line = line.replace("0x08048000", "0x%08x" % newaddress)
	return line

script = map(update, script)
print "\n".join(script)
