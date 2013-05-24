#!/usr/bin/python

import os
import shutil
import subprocess

# I can't make sloccount's --follow option do anything sane.
# Make a dummy directory and copy in all the files from genode, to make
# all the symlinks disappear.

# preconditions:
# check out a fresh tree.
# cd common/crypto-patched; make
# run this program at the root (zoog/) of the tree

class SloccountGenode:
	def __init__(self):
		self.dummydir = "./dummy"
		self.setup_dummy()
		self.count_dummy()
		self.destroy_dummy()

	def setup_dummy(self):
		srcdir = "monitors/genode"
		try:
			shutil.rmtree(self.dummydir)
		except OSError: pass
		dummyroot = os.path.join(self.dummydir, srcdir)
#		print "dummyroot = %s" % dummyroot
		os.makedirs(dummyroot)
		for (root,dirs,files) in os.walk(srcdir):
			if "/.svn" in root:
				continue
#			print root
			try:
				os.mkdir(os.path.join(self.dummydir, root))
			except OSError: pass
			for file in files:
				if "sha2" in file: continue

				srcpath = os.path.join(root, file)
				dstpath = os.path.join(self.dummydir, root, file)
#				print dstpath
				try:
					shutil.copy(srcpath, dstpath)
				except IOError: pass	#missing symlink? Should check more carefully

	def count_dummy(self):
		sp = subprocess.Popen(["sloccount", self.dummydir])
		sp.wait()

	def destroy_dummy(self):
		shutil.rmtree(self.dummydir)

SloccountGenode()
