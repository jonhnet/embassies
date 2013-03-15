#!/usr/bin/python
#
# This script runs midori in the posix environment to get a list of the
# icons it actually touches, and then generates an overlay directory
# suitable for feeding into zftp_create_zarfile to avoid packing all the
# icons into ... rats. It's the cache we care about.
#

import subprocess
import time
import os
import signal
import shutil

def clean():
	try:
		shutil.rmtree("build")
	except:
		pass

def capture():
	os.mkdir("build")
	subproc = subprocess.Popen(["strace", "-f", "-o", "build/strace.out", "../build/midori-pie"])

	sp_ps = subprocess.Popen("ps -ef | grep %d | grep -v strace | grep -v grep | awk '{print $2}'" % subproc.pid, shell=True, stdout=subprocess.PIPE)
	for line in sp_ps.stdout:
		#print line
		midori_pid = int(line)
	#print midori_pid
	time.sleep(8)
	os.kill(midori_pid, signal.SIGTERM)

def select_paths():
	paths = set()
	for line in open("build/strace.out"):
		try:
			field = line.split('"')
			filename = field[1]
		except: continue
		if (filename.startswith("/usr/share/icons/gnome")):
			if (filename.endswith("icon-theme.cache")):
				continue
			if (os.path.isfile(filename)):
				paths.add(filename)
	return paths

def make_overlay(paths):
	os.mkdir("build/overlay")
	for path in paths:
		relpath = "build/overlay"+path
		reldir = os.path.dirname(relpath)
		try:
			os.makedirs(reldir)
		except: pass
		#print relpath
		shutil.copy(path, relpath)
	os.system("gtk-update-icon-cache build/overlay/usr/share/icons/gnome/")
	
def main():
	clean()
	capture()
	paths = select_paths()
	make_overlay(paths)

main()
#grep usr.share.icons.gnome strace.out | awk -F\" '{print $2}' | sort | uniq
#grep usr.share.icons.gnome strace.out | awk -F\" '{print $2}' | sort | uniq | grep 'png$' | sed 's#/usr/share/icons/##' > strace.icons
#tar -f icons.tar -C /usr/share/icons -c --files-from strace.icons
#rm -rf gnome-alternate
#mkdir -p gnome-alternate/usr/share/icons/gnome/
#tar --strip-components=1 -x -f icons.tar -C gnome-alternate/usr/share/icons/gnome/
