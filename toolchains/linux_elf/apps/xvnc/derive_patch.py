#!/usr/bin/python

import re
import os
import glob
import sys

def file_equal(path1, path2):
	rc = os.system("cmp --silent '%s' '%s'" % (path1, path2))
	#print "file_equal returns %d" % rc
	return (rc>>8)==0

def crop_patch(in_path, out_path):
	try:
		ifp = open(in_path)
	except:
		# no in_path => no out_path
		return
	line = ifp.readline()
	assert(line.startswith("---"))
	line = ifp.readline()
	assert(line.startswith("+++"))
	ofp = open(out_path, "w")
	while (True):
		line = ifp.readline()
		if (line==""): break
		ofp.write(line)
	ifp.close()
	ofp.close()
	
def patch_equal(path1, path2):
	crop_patch(path1, path1+".ptmp");
	crop_patch(path2, path2+".ptmp");
	rc = file_equal(path1+".ptmp", path2+".ptmp")
	os.unlink(path1+".ptmp");
	os.unlink(path2+".ptmp");
	return rc;

SRC_PATH = "src/rfb/"
HACKED_PATH = glob.glob("build/source-files/vnc4-*/common/rfb/")[0]
VIRGIN_PATH = glob.glob("build-virgin/source-files/vnc4-*/common/rfb/")[0]

embargo = [".*\.o", ".*\.a", "Makefile", "Makefile.zoog"]

def main():
	update = False
	if (len(sys.argv)>1 and sys.argv[1]=="--update"):
		update = True

	for hacked_path in glob.glob(HACKED_PATH+"*"):
		base = os.path.basename(hacked_path)

		skip = False
		for e in embargo:
			regex = "^"+e+"$"
			if (re.compile(regex).match(base)):
				#print "embargo %s" % base
				skip = True
			#print "%s escapes embargo %s" % (base, regex)
		if (skip):
			continue

		virgin_path = VIRGIN_PATH+base
		src_path = SRC_PATH+base+".patch"

		debug = False
#		debug = base=="VNCZoogDisplay.cxx"
		if (debug):
			print "hacked: ", hacked_path
			print "virgin: ", virgin_path

		need_patch = False

		if (os.path.exists(virgin_path)):
			if (file_equal(hacked_path, virgin_path)):
				# file matches stock system file
				if (os.path.exists(src_path)):
					print "%s seems to be redundant with a virgin file" % base
				else:
					if (debug):
						print "%s matches a virgin file" % base
			else:
				if (debug):
					print "need_patch due to unequal files"
				need_patch = True
		else:
			if (debug):
				print "need_patch due to file is absent from virgin"
			need_patch = True

		ref_path = src_path+".ref"
		if (need_patch):
			if (not os.path.exists(src_path)):
				print "Need to create patch for %s" % src_path
				if (update):
					os.system("diff -Nu '%s' '%s' > '%s'" % (
						virgin_path, hacked_path, src_path))
			else:
				cmd = "diff -Nu '%s' '%s' > '%s'" % (
					virgin_path, hacked_path, ref_path);
				os.system(cmd)
				if (patch_equal(ref_path, src_path)):
					if (debug):
						print "(%s) Patch equal" % cmd
						#sys.exit(1)
					pass
				else:
					print "Need update for %s" % src_path
					if (update):
						os.rename(ref_path, src_path)
				try:
					os.unlink(ref_path)
				except:
					pass

main()
