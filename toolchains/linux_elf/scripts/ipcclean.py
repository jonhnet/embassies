#!/usr/bin/python

# The xax_port_monitor, when crashy, leaves ipc segments lying around.
# This script removes all ipc segments that no process is attached to.

import os

def clean_one(line):
	fields = line.split()
	shmid = int(fields[1])
	nattch = int(fields[5])
	if (nattch==0):
		os.system("ipcrm -m %d" % shmid)
	
for line in os.popen("ipcs -m", "r"):
	try:
		clean_one(line)
	except: pass
