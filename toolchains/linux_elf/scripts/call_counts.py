#!/usr/bin/python

import subprocess

def read_legend(file, pattern):
	sp = subprocess.Popen(
		["cat "+file+" | "+pattern],
		shell=True,
		stdout=subprocess.PIPE)
	legend = map(lambda l: l.strip(), sp.stdout)
	return legend

def read_dbg_legend():
	return read_legend(
		"../../../common/ifc/pal_abi/pal_abi.h",
		"grep zf_zoog ../../../common/ifc/pal_abi/pal_abi.h | sed 's/^.*zoog_//' | sed 's/;$//'")

def read_kvm_legend():
	return read_legend(
		"../../../monitors/linux_kvm/common/linux_kvm_protocol.h",
		"grep 'xp_' | sed 's/[,=].*$//' | sed 's/^\s//'")


def display(legend):
	for value in open("call_counts"):
		(call,idx,count) = value.split()
		idx = int(idx)
		count = int(count)
		if (idx < len(legend)):
			print "%-40s: %s" % (legend[idx], count)
		elif (count>0):
			print idx, len(legend), count
			raise Exception("unknown idx!?")

#display(read_kvm_legend())
display(read_dbg_legend())
