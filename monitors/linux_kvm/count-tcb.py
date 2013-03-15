#!/usr/bin/python

import subprocess
import os
import shutil

def scan(dir):
	p = subprocess.Popen("../../toolchains/linux_elf/scripts/find_dependencies.pl %s/build/*.dep" % dir, shell=True, stdout=subprocess.PIPE)
	files = p.stdout.readlines()
	outset = set()
	for file in files:
		file = file.strip()
		if (file[0]!='/'):
			file = dir+"/"+file
		outset.add(file)
	return outset

s1 = scan("monitor")
print len(s1)
s1 = s1.union(scan("coordinator"))
print len(s1)
dirs = set()
workdir = "county/"
try:
	shutil.rmtree(workdir)
except:
	pass
os.mkdir(workdir)
for f in s1:
	p = os.path.abspath(f)
	if (p.startswith("/home")):
		try:
			shutil.copyfile(p, workdir+os.path.basename(p))
		except:
			pass
subprocess.Popen("sloccount %s" % workdir, shell=True)

