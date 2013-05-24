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

ZOOG_ROOT = os.path.abspath(os.path.dirname(sys.argv[0])+"/../../..")+"/"
try:
	fp=open("%s/.zoog_tunid" % os.getenv("HOME"), "r")
	_zoog_tunid=int(fp.read())
	fp.close()
except:
	raise Exception("Cannot learn zoog_tunid")
TIDY_ON_CLOSE = False

NUM_REPETITIONS = 10

ZCZ_DIR = ZOOG_ROOT + "toolchains/linux_elf/zftp_create_zarfile/"
ZARFILE_REL = "build/midori.zarfile"
TWEAKED_ZARFILE_REL = "build/%s.tweaked-zarfile"
ZARFILE = ZCZ_DIR + ZARFILE_REL
TWEAKED_ZARFILE = ZCZ_DIR + TWEAKED_ZARFILE_REL
hostname = os.uname()[1]
ZARFILE_PATHS = ZCZ_DIR + "midori.paths."+hostname

MEASURE_POSIX = ZOOG_ROOT + "toolchains/linux_elf/measure_posix/build/measure_posix"
MIDORI_DIR = ZOOG_ROOT + "toolchains/linux_elf/apps/midori/"
MIDORI_PIE = MIDORI_DIR + "build/midori-pie"
MIDORI_ARG = MIDORI_DIR + "%s.html"

class AbstractEx(Exception): pass

class SubprocessExpiredEx(Exception):
	def __init__(self, rc):
		self.rc = rc

	def __str__(self):
		return "Process exited (%s)." % repr(self.rc)

class Procedure:
	def __init__(self, name):
		self.name = name

	def __repr__(self):
		return self.name

class CmdSpec:
	def __init__(self, args, env=None):
		self.args = args
		self.env = env

	def __repr__(self):
		return " ".join(self.args)

class OutputNameAllocator:
	def __init__(self):
		self.table = {}

	def name(self, base):
		if (base not in self.table):
			self.table[base] = 0
		result = self.table[base]
		self.table[base] += 1
		return "%s-%s" % (base, result)

output_name_allocator = OutputNameAllocator()

class ShellProc(Procedure):

	# abstract interface: things child must supply
	def cmd(self): raise AbstractEx()
	def cmds(self): return [ self.cmd() ]
	def probe_running(self): raise AbstractEx()
	def before_start_hook(self): pass
	def interactive(self): return False
	
	def __init__(self, name, use_gdb=False):
		Procedure.__init__(self, name)
		self.subprocs = []
		self.workdir = None
		self.use_gdb = use_gdb

	def open(self):
		self.workdir = "./output/%s" % output_name_allocator.name(self.name)
		self.tidy()
		os.makedirs(self.workdir)

		kkw = {}
		kkw["cwd"] = self.workdir
	
		cmds = self.cmds()
		for i in range(len(cmds)):
			cmd_spec = cmds[i]

			# my kingdom for static types
			if (type(cmd_spec)!=types.InstanceType):
				sys.stderr.write("bogus: %s\n" % cmd_spec)
				assert(False)

			if (i==0):
				suffix = ""
			else:
				suffix = "%d" % i
			if (self.use_gdb):
				fp = open("%s/run.cmd" % self.workdir, "w")
				fp.write("run\n")
				fp.close()
				cmd_spec = CmdSpec(
					["gdb", "-command", "run.cmd", cmd_spec.args[0], "--args"] + cmd_spec.args)
			else:
				kkw["stdout"] = open("%s/stdout%s" % (self.workdir, suffix), "w")
				kkw["stderr"] = open("%s/stderr%s" % (self.workdir, suffix), "w")

			self.before_start_hook()
			print "Starting %s %s..." % (self.name, cmd_spec)
			kkw["env"] = cmd_spec.env
			subproc = subprocess.Popen(cmd_spec.args, **kkw)
			self.subprocs.append(subproc)
			print "  ... pid %d..." % (subproc.pid)
			self.wait_until_started(subproc)
			print "Started %s." % self.name

	def shell_failure_okay(self):
		return False

	def time_limit(self):
		return None

	def wait_until_started(self, subproc):
		poll_period = 1.0	# seconds
		elapsed_time = 0;
		time_limit = self.time_limit()
		while (True):
			if (time_limit!=None and elapsed_time > time_limit):
				self.kill(subproc)
				time.sleep(0.05)
				print("Subprocess timed out; killing")
				# then roll along and probe as usual, possibly allowing
				# shell_failure_okay.

			rc = subproc.poll()
			if (rc!=None):
				if (rc==False):
					# process exited cleanly
					break
				elif (self.shell_failure_okay() and self.probe_running()):
					print "Exited with failure, but got results anyway"
				else:
					raise SubprocessExpiredEx("%s: %s" % (self.name, rc))
			if (self.probe_running()):
				break
			sys.stdout.write(".")
			sys.stdout.flush()
			time.sleep(poll_period)
			elapsed_time += poll_period

	def kill(self, subproc):
		subproc.kill()

	def close(self):
		print "Closing %s" % self.name
		for subproc in self.subprocs[::-1]:
			try:
				self.kill(subproc)
				subproc.wait()
			except OSError, ex:
				if (ex.errno == 3):
					pass # aready dead
				else:
					raise
			print "Stopped %s; pid %d" % (self.name, subproc.pid)
		if (TIDY_ON_CLOSE):
			self.tidy()

	def tidy(self):
		try:
			if (self.workdir != None):
				shutil.rmtree(self.workdir)
		except OSError:
			pass

class LinuxDbgMonitor:
	def name(self): return "Dbg"
	def monitor_cmds(self):
		return [
			CmdSpec([ZOOG_ROOT + "toolchains/linux_elf/scripts/ipcclean.py"]),
			CmdSpec([ZOOG_ROOT + "monitors/linux_dbg/monitor/build/xax_port_monitor"]) ]
	def pal_cmd(self, signed_binary_path):
		return CmdSpec([
			ZOOG_ROOT + "monitors/linux_dbg/pal/build/xax_port_pal",
			signed_binary_path ])
	def socket_name(self):
		return "/tmp/xax_port_socket:%d" % _zoog_tunid
	def setup_cmd(self):
		return None

class LinuxKvmSetup(ShellProc):
	def __init__(self):
		ShellProc.__init__(self, "LinuxKvmSetup")

	def open(self):
		p = subprocess.Popen("lsmod | grep kvm_", shell=True)
		rc = p.wait()
		if (rc==False):
			# looks like kvm is actually installed
			return
		else:
			ShellProc.open(self)

	def cmds(self):
		cpuinfo = open("/proc/cpuinfo").read()
		if (" svm " in cpuinfo):
			type = "kvm_amd"
		elif (" vmx " in cpuinfo):
			type = "kvm_intel"
		else:
			raise Exception("Cannot find kvm extension type")
		return [
			CmdSpec([ "sudo", "modprobe", "kvm" ]),
			CmdSpec([ "sudo", "modprobe", type ])
			]

	def probe_running(self):
		time.sleep(0.1)
		return False

class LinuxKvmMonitor:
	def name(self): return "Kvm"
	def monitor_cmds(self):
		return [
			CmdSpec([ZOOG_ROOT + "toolchains/linux_elf/scripts/ipcclean.py"]),
			CmdSpec([
				ZOOG_ROOT + "monitors/linux_kvm/coordinator/build/zoog_kvm_coordinator" ])
			]
	def pal_cmd(self, signed_binary_path):
		return CmdSpec([
			ZOOG_ROOT + "monitors/linux_kvm/monitor/build/zoog_kvm_monitor",
			"--image-file",
			signed_binary_path,
			"--wait-for-core", "false"
			])
	def socket_name(self):
		return "/tmp/zoog_kvm_coordinator:%d" % _zoog_tunid
	def setup_cmd(self):
		return LinuxKvmSetup()

class Monitor(ShellProc):
	def __init__(self, monitor_type):
		ShellProc.__init__(self, "Monitor"+monitor_type.name())
		self.monitor_type = monitor_type

	def before_start_hook(self):
		try:
			os.mkdir("%s/build" % (self.workdir))
			time.sleep(1)
				# give the last guy time to die properly;
				# trying to evade a racey crashy thing I haven't diagnosed
		except:
			pass
		setup_cmd = self.monitor_type.setup_cmd()
		if (setup_cmd!=None):
			setup_cmd.open()
			setup_cmd.close()

	def cmds(self):
		return self.monitor_type.monitor_cmds()

	def probe_running(self):
		try:
			sockname = self.monitor_type.socket_name()
			s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
			s.connect(sockname)
			s.close()
			return True
		except Exception, ex:
			#print "failure is ",ex
			return False


def broadcast_socket(address):
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, True)
	s.settimeout(1)
	s.bind(("10.%d.0.1" % _zoog_tunid, 0))
	return s
	
def probe_zftp(address):
	try:
		s = broadcast_socket(address)

		# form a zftp_request_packet for the zero hash
		hash_len = 20
		url_hint_len = 0;
		msg = struct.pack("!HH", hash_len, url_hint_len)
		msg += ('\0'*hash_len)
		msg += struct.pack("!LLLLLL", 0, 0, 0, 0, 0, 0)
			# num_tree_locations
			# padding_request
			# compression.state
			# compression.seq
			# data_start
			# data_end

		s.sendto(msg, (address, 7662))
		x = s.recv(1024)
		print "Recvd %d bytes" % len(x)
		s.close()
		return True
	except Exception, ex:
		#print "failure is ",ex
		return False

def probe_keyval(address):
	try:
		s = broadcast_socket(address)

		# form a keyval request for a dummy key
		msg = chr(1) + chr(6)*8 + chr(0)*4;
		s.sendto(msg, (address, 4242))
		x = s.recv(1024)
		print "Recvd %d bytes" % len(x)
		s.close()
		return True
	except Exception, ex:
		#print "failure is ",ex
		return False

class ZftpBackend(ShellProc):
	def __init__(self, log_paths=None):
		self.log_paths = log_paths
		name = "ZftpBackend"
		if (self.log_paths!=None):
			name+="_logging"
		ShellProc.__init__(self, name)

	def before_start_hook(self):
		if (self.log_paths!=None):
			if (os.path.exists(self.log_paths)):
				os.unlink(self.log_paths)

	def cmd(self):
		backend_dir = ZOOG_ROOT + "toolchains/linux_elf/zftp_backend/"
		backend_bin = backend_dir + "build/zftp_backend"
		arg_list = [
			backend_bin,
			"--origin-filesystem",
			"true",
			"--origin-reference",
			"true",
			"--listen-zftp",
			"tunid",
			"--listen-lookup",
			"tunid",
			"--index-dir",
			backend_dir + "zftp_index_origin"]
		if (self.log_paths != None):
			arg_list += ["--log-paths", self.log_paths]
		return CmdSpec(arg_list)

	def probe_running(self):
		return probe_zftp("10.%d.0.1" % _zoog_tunid)

class ZftpZoog(ShellProc):
	def __init__(self, monitor_type):
		ShellProc.__init__(self, "ZftpZoog")
		self.monitor_type = monitor_type

	def cmd(self):
		arg = ZOOG_ROOT + "toolchains/linux_elf/zftp_zoog/build/zftp_zoog.signed"
		return self.monitor_type.pal_cmd(arg)

	def probe_running(self):
		return probe_zftp("10.%d.255.255" % _zoog_tunid)

class KeyVal(ShellProc):
	def __init__(self, monitor_type):
		ShellProc.__init__(self, "KeyVal")
		self.monitor_type = monitor_type

	def cmd(self):
		arg = ZOOG_ROOT + "toolchains/linux_elf/elf_loader/build/elf_loader.keyval.signed"
		return self.monitor_type.pal_cmd(arg)

	def probe_running(self):
		return probe_keyval("10.%d.255.255" % _zoog_tunid)

class ZftpCreateZarfile(ShellProc):
	def __init__(self, vendor):
		ShellProc.__init__(self, "ZftpCreateZarfile")
		self.vendor = vendor

	def cmd(self):
		return CmdSpec([ "make", "-C", ZCZ_DIR, ZARFILE_REL, TWEAKED_ZARFILE_REL % self.vendor])

	def probe_running(self):
		return False

class CaptureZarfile(Procedure):
	def __init__(self, monitor_type, vendor):
		Procedure.__init__(self, "CaptureZarfile")
		self.monitor_type = monitor_type
		self.vendor = vendor
		
	def open(self):
		print "capturing neutered."
		return # this code is rotted; can't cope with mmaps

		tweaked_zarfile = TWEAKED_ZARFILE % self.vendor
		if (os.path.exists(ZARFILE) and os.path.exists(tweaked_zarfile)):
			print "zarfiles already present"
			return

		if (os.path.exists(ZARFILE_PATHS)):
			print "using existing paths; not capturing new."
		else:
			zb = ZftpBackend(log_paths=ZARFILE_PATHS)
			midori = MidoriZoog(monitor_type)
			execute_steps([zb, midori])
		
		zcz = ZftpCreateZarfile(self.vendor)
		execute_steps([zcz])

	def close(self):
		pass

class Span:
	def __init__(self, desc, start_name, end_name):
		self.desc = desc
		self.start_name = start_name
		self.end_name = end_name

	def __hash__(self):
		return hash(self.desc)

	def __cmp__(self, other):
		return cmp(self.desc, other.desc)

	def __repr__(self):
		return self.desc

class MidoriCommon(ShellProc):
	def __init__(self, name):
		ShellProc.__init__(self, name)
		self._runtimes = {}

	def collect_perfs(self):
		perfs = {}
		try:
			fn = "%s/%s" % (self.workdir, self.debug_perf_file())
			#print "reading perfs from %s" % fn
			fp = open(fn)
			start = None
			for line in fp:
				line = line.strip()
				if (not line.startswith("mark_time")):
					# be robust for when we want to read data from
					# debug_stderr
					continue
				(label, timehex, event) = line.split(' ', 2)
				timeint = int(timehex, 16)
				if (event == "start"):
					start = timeint
					perfs["start"] = 0.0
				elif (start==None):
					print "debug_perf starts with '%s'" % line
				elif (event in perfs):
					print "dropping duplicate perf event %s" % event
				else:
					perfs[event] = timeint - start
		except Exception, ex:
	#		print "collect_perfs ex %s" % ex
	#		traceback.print_exc()
			pass
		return perfs

	def last_perf_string(self):
		return "midori_load_finished"

	def requested_spans(self):
		return [
			Span("10_zoog_start", "start", "request_zarfile"),
			Span("11_fetch", "request_zarfile", "mac_lookup_start"),
			# hot path
			Span("20_hot_mac_lookup", "mac_lookup_start", "mac_verify_start"),
			Span("21_hot_mac_verify", "mac_verify_start", "mac_verify_end"),
			Span("22_hot_app_boot", "mac_verify_end", "midori_load_provisional"),
			# warm path
			Span("50_warm_mac_lookup", "mac_lookup_start", "hash_start"),
			Span("60_warm_hash", "hash_start", "mac_start"),
			Span("70_warm_mac", "mac_start", "mac_end"),
			Span("80_warm_app_boot", "mac_end", "midori_load_provisional"),
			# common end path
			Span("90_page_load", "midori_load_provisional", self.last_perf_string()),
			Span("99_total", "start", self.last_perf_string()),
			]

	def get_one_perf_point(self, name):
		return float(self.perfs[name])*1.0e-9

	def parse_runtimes(self):
		for span in self.requested_spans():
			try:
				v0 = self.get_one_perf_point(span.start_name)
				v1 = self.get_one_perf_point(span.end_name)
				self._runtimes[span] = v1-v0
			except:
				#print "No data for %s; have %s" % (span, self.perfs)
				pass
	

	def probe_running(self):
		self.perfs = self.collect_perfs()
		last_perf_str = self.last_perf_string()
		#print "last_perf_str %s perfs %s" % (last_perf_str, self.perfs)
		done = (last_perf_str in self.perfs)
		#print "done %s; perfs is %s" % (done, self.perfs)
		if (done):
			self.parse_runtimes()
			#print self._runtimes
		return done

	def close(self):
		# app may not be Midori (this script is a disaster); it may
		# have exited sanely, so we need to go fetch perfs one last time.
		self.perfs = self.collect_perfs()
		#print "perfs %s" % self.perfs
		self.parse_runtimes()
		#print "runtimes %s" % self._runtimes
		ShellProc.close(self)

	def runtimes(self):
		return self._runtimes

class MidoriZoog(MidoriCommon):
	def __init__(self, jobname, monitor_type, vendor):
		MidoriCommon.__init__(self, jobname)
		self.monitor_type = monitor_type
		self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/elf_loader/build/elf_loader.%s.signed" % vendor

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_perf"

	def shell_failure_okay(self):
		return True

	def time_limit(self):
		return 10	# seconds until it's either good or dead

class MidoriPosix(MidoriCommon):
	def __init__(self, vendor, sudouser=None):
		MidoriCommon.__init__(self, "MidoriPosix")
		self.vendor = vendor
		self.sudouser = sudouser

	def before_start_hook(self):
		os.chmod(self.workdir, 0777)

	def cmd(self):
		cmd_block = [ MEASURE_POSIX, MIDORI_PIE, MIDORI_ARG % self.vendor ]

		if (self.sudouser!=None):
			# switch users to one whose routes have been mangled to a local
			# dead DNS and TCP RST, so no network variability is present.
			cmd_block = ["sudo", "-u", self.sudouser] + cmd_block

		return CmdSpec(cmd_block)

	def kill(self, subproc):
		if (self.sudouser!=None):
			# kill has to be as root, because we're killing the sudo itself,
			# not the net-restricted process. (We could try to look up its
			# pid, but bleah.
			subproc = subprocess.Popen(["sudo", "kill", "%d"%subproc.pid])
			subproc.wait()
		else:
			MidoriCommon.kill(self, subproc)

	def debug_perf_file(self): return "debug_perf_posix"

def execute_steps(steps):
	opened = []

	try:
		for step in steps:
			opened.append(step)
			step.open()
	except Exception, ex:
		#print "open ex"
		#traceback.print_exc()
		raise
	finally:
		for step in opened[::-1]:
			try:
				step.close()
			except:
				print "close ex"
				traceback.print_exc()

class Sleep(Procedure):
	def __init__(self, duration):
		Procedure.__init__(self, "Sleep")
		self.duration = duration
		
	def open(self):
		time.sleep(self.duration)

	def close(self):
		pass

class EvaluateMidori(Procedure):
	def __init__(self, name):
		Procedure.__init__(self, name)
		self.data = []
		
	def cool_time(self):
		return 2

	def open(self):
		round = 0
		while (round < NUM_REPETITIONS):
			_experiment = self.experiment()
			steps = self.prep() + [Sleep(self.cool_time()), _experiment]
			try:
				execute_steps(steps)
				round += 1
			except SubprocessExpiredEx, ex:
				print "Failure; skipping (%s)" % ex
				continue
			#print "Round %d: %s" % (round, _experiment.runtimes())
			self.data.append(_experiment.runtimes())
		
	def close(self):
		fp = open("%s.runtimes" % self.name, "w")
		def array_to_string(a):
			return ",".join(map(str, a))
		for span in self.data[0]:
			spanwise = map(lambda d: d[span], self.data)
			result = self.name+":"+str(span)+","+array_to_string(map(str, spanwise))
			fp.write(result+"\n")
		fp.close()
		pass

class EvalHotSite(EvaluateMidori):
	def __init__(self, monitor_type, vendor):
		EvaluateMidori.__init__(self, "EvalHotSite-%s" % vendor)
		self.monitor_type = monitor_type
		self.vendor = vendor

	def prep(self): return []

	def experiment(self): return MidoriZoog(self.name+"-exp", self.monitor_type, "%s-hot" % self.vendor)

		
class EvalWarmSite(EvaluateMidori):
	def __init__(self, monitor_type, vendor):
		EvaluateMidori.__init__(self, "EvalWarmSite-%s" % vendor)
		self.monitor_type = monitor_type
		self.vendor = vendor

	def prep(self):
		return [
			ZftpZoog(self.monitor_type),
			KeyVal(self.monitor_type),
			#MidoriZoog(self.name+"-prep1", self.monitor_type, self.vendor),
			MidoriZoog(self.name+"-prep2", self.monitor_type, "%s-warm" % self.vendor),
			]

	def experiment(self): return MidoriZoog(self.name+"-exp", self.monitor_type, "%s-hot" % self.vendor)

class CleanPosixMidori(Procedure):
	def __init__(self, sudouser = None):
		Procedure.__init__(self, "CleanPosixMidori")
		self.sudouser = sudouser
		
	def open(self):
		if (self.sudouser==None):
			cmd_block = ["rm", "-rf", "~/.config/midori"]
		else:
			cmd_block = [ "sudo", "-u", self.sudouser,
				"rm", "-rf", "/home/%s/.config/midori" % self.sudouser ]
		cmd_str = " ".join(cmd_block)
		print cmd_str
		p = subprocess.Popen(cmd_str, shell=True)

	def close(self):
		pass

class EvalPosix(EvaluateMidori):
	def __init__(self, vendor, sudouser):
		EvaluateMidori.__init__(self, "EvalPosix-%s" % vendor)
		self.vendor = vendor
		self.sudouser = sudouser

	def prep(self):
		return [ CleanPosixMidori(self.sudouser) ]

	def experiment(self):
		return MidoriPosix(self.vendor, self.sudouser)

class PrefetchHotMidori(MidoriZoog):
	# load a copy of the app just to be sure everything's warmed up.
	# (contrast with "warm" tests, where we're not getting things all the
	# way "hot" before measuring.)
	def __init__(self, monitor_type, vendor):
		MidoriZoog.__init__(self, "PrefetchHotMidori", monitor_type, "%s%s" % (vendor, self.hotness()))

	def hotness(self):
		return "-hot"

	def open(self):
		print "In PrefetchHotMidori"
		while (True):
			try:
				MidoriZoog.open(self)
				break
			except SubprocessExpiredEx:
				print "Failure; retrying prefetch"
				continue

class TopHotCache:
	def __init__(self, monitor_type, vendor):
		steps = [
			Monitor(monitor_type),
			CaptureZarfile(monitor_type, vendor),
			ZftpBackend(),
			KeyVal(monitor_type),
			ZftpZoog(monitor_type),
			PrefetchHotMidori(monitor_type, vendor),
			EvalHotSite(monitor_type, vendor) ]
		execute_steps(steps)

class TopWarmCache:
	def __init__(self, monitor_type, vendor):
		steps = [
			Monitor(monitor_type),
			CaptureZarfile(monitor_type, vendor),
			ZftpBackend(),
			EvalWarmSite(monitor_type, vendor)
		]
		execute_steps(steps)

##############################################################################
class StartDemoApp(PrefetchHotMidori):
	def __init__(self, monitor_type, vendor):
		PrefetchHotMidori.__init__(self, monitor_type, vendor)

	def hotness(self):
		return ""

	# overrides MidoriCommon
	def close(self):
		return

	# overrides MidoriZoog
	def time_limit(self):
		return 60*60

class DemoStart:
	def __init__(self, monitor_type):
		steps = [
			Monitor(monitor_type),
			#CaptureZarfile(monitor_type, vendor),
			ZftpBackend(),
			KeyVal(monitor_type),
			ZftpZoog(monitor_type),
			#ResultsSearch(monitor_type),
			StartDemoApp(monitor_type, "results_search_com"),
			StartDemoApp(monitor_type, "tabman"),
			#EvalHotSite(monitor_type, vendor)
			]
		execute_steps(steps)
