#!/usr/bin/python
#
# fetch-code.py <dest_path> <origin_url> <cache_path>
#
# Fetches code, either from a cached download, or from an
# origin server if no local copy is available.
# (This lets me build directories that can be rebuilt without
# an Internet connection.)
#
import os
import shutil
import subprocess
import sys

destination_path = sys.argv[1]
origin_url = sys.argv[2]
cache_path = sys.argv[3]

if (os.path.exists(cache_path)):
	shutil.copy(cache_path, destination_path)
else:
	p = subprocess.Popen(["wget", "-O", destination_path, origin_url])
	p.wait()

# touch file's mtime to ensure it appears fresh
os.utime(destination_path, None)
