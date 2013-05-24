#!/usr/bin/python

import sys
import struct
import socket
import time
import os
import subprocess
import shutil
import traceback
import types

from StartTimeCore import *

from LeanLoaderPerf import *
from ZHyperoidPerf import *
from GimpPerf import *
from GimpStart import *
from ZftpPerf import *

#////////////////////////////////////////////////////////////////////////////

class TopPosix:
	def __init__(self, vendor):
		sudouser = "net-restricted"
#		sudouser = None
		steps = [ EvalPosix(vendor, sudouser) ]
		execute_steps(steps)

#////////////////////////////////////////////////////////////////////////////

#monitor_type = LinuxKvmMonitor()
monitor_type = LinuxDbgMonitor()

if (False):
	TopHotCache(monitor_type, "midori_hot")
	TopWarmCache(monitor_type, "midori_hot")
	TopPosix("vendor-a")

if (False):
#	TopLeanLoader(monitor_type)
	TopZhyperoid(monitor_type)

if (False):
	TopGimpPerf(monitor_type)
	TopGimpPerfPosix(monitor_type)

if (False):
#	for vendor in ("ebay",):
	for vendor in ("ebay", "microsoft", "reddit", "craigslist", ):
		TopHotCache(monitor_type, vendor)
		TopWarmCache(monitor_type, vendor)
#		TopPosix(vendor)

if (False):
	TopZftpPerf(monitor_type)
	TopZftpPerfPosix(monitor_type)

if (False):
	TopGimpStart(monitor_type)
	TopGimpStartPosix(monitor_type)

if (True):
	DemoStart(monitor_type)
