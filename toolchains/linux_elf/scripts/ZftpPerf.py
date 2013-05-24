import glob
from StartTimeCore import *

ZFTP_TIME_FETCH = ZOOG_ROOT + "toolchains/linux_elf/zftp_time_fetch/build/zftp_time_fetch.signed"
ZFTP_TARGET_LINK = ZOOG_ROOT + "toolchains/linux_elf/zftp_time_fetch/test.zarfile"

class PrepLink(ShellProc):
	def __init__(self, zarfile):
		self.zarfile = zarfile
		ShellProc.__init__(self, "PrepLink")
	def cmds(self):
		return [
			CmdSpec([ "rm", ZFTP_TARGET_LINK ]),
			CmdSpec([ "ln", "-s", self.zarfile, ZFTP_TARGET_LINK ])
			]
	def probe_running(self): return False

class ZftpPerf(MidoriCommon):
	def __init__(self, name, monitor_type, elf_loader_config = None):
		MidoriCommon.__init__(self, name)
		self.monitor_type = monitor_type
		if (elf_loader_config == None):
			self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/zftp_time_fetch/build/zftp_time_fetch.signed"
		else:
			self.elf_loader_config = elf_loader_config

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_perf"

	def last_perf_string(self): return "done"

	def requested_spans(self):
		return [
			Span("30_fetch", "start", "mac_lookup_start"),
			Span("50_mac_lookup", "mac_lookup_start", "mac_lookup_end"),
			Span("60_mac_verify", "mac_verify_start", "mac_verify_end"),
			Span("99_total", "start", "done"),
			]

class PrefetchHotZftp(ZftpPerf):
	# load a copy of the app just to be sure everything's warmed up.
	# (contrast with "warm" tests, where we're not getting things all the
	# way "hot" before measuring.)
	def __init__(self, monitor_type, base):
		ZftpPerf.__init__(self, self.__class__.__name__+"-"+base, monitor_type)
			#TODO will need appropriate path here -- or, symlink! Bang.

	def open(self):
		print "In %s" % self.__class__.__name__
		while (True):
			try:
				ZftpPerf.open(self)
				break
			except SubprocessExpiredEx, ex:
				print "Failure (%s); retrying prefetch" % ex
				continue


class EvalZftpPerf(EvaluateMidori):
	def __init__(self, monitor_type, base):
		self.base = base
		EvaluateMidori.__init__(self, "EvalZftpPerf-%s" % base)
		self.monitor_type = monitor_type

	def cool_time(self):
		return 4

	def prep(self): return []

	def experiment(self): return ZftpPerf("ZftpPerf-%s" % self.base, self.monitor_type)
		
class OneZftpPerf:
	def __init__(self, monitor_type, zarfile, base):
		self.monitor_type = monitor_type
		self.zarfile = zarfile
		self.base = base

	def open(self):
		steps = [
			Monitor(self.monitor_type),
			ZftpBackend(),
			ZftpZoog(self.monitor_type),
			KeyVal(self.monitor_type),
			PrepLink(self.zarfile),
			PrefetchHotZftp(self.monitor_type, self.base),
			PrefetchHotZftp(self.monitor_type, self.base),
			EvalZftpPerf(self.monitor_type, self.base)
		]
		execute_steps(steps)

	def close(self):
		pass

def zarfile_set():
	whole = glob.glob("/home/jonh/posix-initial-zar/*.zar")
	def small_enough(file):
		return os.path.getsize(file) < (140<<20)
	whole = filter(small_enough, whole)
	whole = filter(lambda w:
		os.path.basename(w) =="xfig.zar"
		, whole)
	whole.sort()
	print whole
	return whole
	#return ( "/home/jonh/posix-initial-zar/abiword.zar", )

def zar_to_base(zarfile):
	return os.path.basename(zarfile).split(".")[0]
	
class TopZftpPerf:
	def __init__(self, monitor_type):
		steps = []
		for zar in zarfile_set():
			steps += [
				OneZftpPerf(monitor_type, zar, zar_to_base(zar))
				]
		execute_steps(steps)

class ZftpPerfPosix(MidoriCommon):
	def __init__(self):
		MidoriCommon.__init__(self, "ZftpPerfPosix")

	def cmd(self):
		return CmdSpec([
			ZOOG_ROOT + "toolchains/linux_elf/zftp_time_posix/build/zftp_time_posix",
			"../image"])

	def debug_perf_file(self): return "debug_perf_posix"

	def last_perf_string(self): return "done"

	def requested_spans(self):
		return [
			Span("99_total", "start", "done"),
			]

class PrepImage(ShellProc):
	def __init__(self, zarfile):
		self.zarfile = zarfile
		ShellProc.__init__(self, "PrepImage")
	def cmd(self):
		return CmdSpec([
			ZOOG_ROOT  + "toolchains/linux_elf/zftp_time_posix/prep_image.py",
			"../image/",
			self.zarfile])
	def probe_running(self): return False

class EvalZftpPerfPosix(EvaluateMidori):
	def __init__(self, which):
		EvaluateMidori.__init__(self, "EvalZftpPerfPosix-%s" % which)

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return ZftpPerfPosix()

class TopZftpPerfPosix:
	def __init__(self, vendor):
		steps = []
		for zar in zarfile_set():
			steps += [
				PrepImage(zar),
				EvalZftpPerfPosix(zar_to_base(zar)),
				]
		execute_steps(steps)

