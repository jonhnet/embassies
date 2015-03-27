#!/usr/bin/python

import shutil
import sys
import os
import glob
import subprocess

ZOOG_ROOT="/home/jonh/zoog/"

class PrepImage:
	def __init__(self, image_dir, zarfile):
		self.image_dir = image_dir
		self.unpack(zarfile)
		self.sift()

	def unpack(self, zarfile):
	#	ZARFILE=ZOOG_ROOT+"toolchains/linux_elf/zftp_create_zarfile/build/gnumeric.zarfile"
	#	ZARFILE="/tmp/abiword.zar"

		try:
			shutil.rmtree(self.image_dir)
		except OSError:
			pass

		os.mkdir(self.image_dir)

		cmd = [
			ZOOG_ROOT+"toolchains/linux_elf/zftp_create_zarfile/build/zarfile_tool",
			"--flat-unpack",
			self.image_dir,
			"--zarfile",
			zarfile]
		sp = subprocess.Popen(cmd);
		rc = sp.wait()
		if (rc!=0):
			print " ".join(cmd)

	def sift(self):
		MMAP_DIR=self.image_dir+"mmap"
		READ_DIR=self.image_dir+"read"
		os.mkdir(MMAP_DIR)
		os.mkdir(READ_DIR)
		for f in glob.glob(self.image_dir+"/*.chunk"):
			fp = open(f, "rb")
			prefix = fp.read(4)[1:]
			if (prefix=="ELF"):
				shutil.move(f, MMAP_DIR)
			else:
				shutil.move(f, READ_DIR)

PrepImage(sys.argv[1], sys.argv[2])
