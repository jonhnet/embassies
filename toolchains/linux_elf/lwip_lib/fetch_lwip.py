#!/usr/bin/python

raise Exception("this script deprecated; svn rm it.")

import sys
import os

def exsystem(cmd):
	print cmd
	rc = os.system(cmd)
	if (rc!=0):
		raise Exception("cmd failed: %s" % cmd)

def main():
	download_site = sys.argv[1]
	target_dir = sys.argv[2]
	version = sys.argv[3]
	os.makedirs(target_dir)
	for package in ["lwip", "contrib"]:
		fn = "%s%s.zip" % (package, version)
		exsystem("wget -q %s/%s -O %s/%s" % (download_site, fn, target_dir, fn))
		exsystem("(cd %s && unzip -q %s)" % (target_dir, fn))
	os.symlink("lwip%s" % version, "%s/lwip" % target_dir);
	fp = open("%s/unpack.stamp" % target_dir, "w")
	fp.close()
	sys.exit(0)

main()
