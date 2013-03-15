#!/usr/bin/python

import glob
import os
import subprocess

for path in glob.glob("../certs/*.chain"):
	bn = os.path.basename(path)
	vendor_desc = bn[:-6]
	sp = subprocess.Popen(["../build/cert_to_hash", path], stdout=subprocess.PIPE);
	hashv = sp.stdout.read()
	hashv = hashv.strip()
	fp = open(hashv, "w")
	fp.write(bn)
	fp.close()
