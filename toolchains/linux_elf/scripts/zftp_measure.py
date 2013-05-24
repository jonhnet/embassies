#!/usr/bin/python

import os
import subprocess
import sys

ZOOG_ROOT = os.path.abspath(os.path.dirname(sys.argv[0])+"/../../..")+"/"

zftp_backend_path = ZOOG_ROOT+"toolchains/linux_elf/zftp_backend/build/zftp_backend"
zftp_index_path = ZOOG_ROOT+"toolchains/linux_elf/zftp_backend/zftp_index-origin"
zarfile_dir = ZOOG_ROOT+"toolchains/linux_elf/zftp_create_zarfile/build/"

bit_bucket = open("/dev/null", "w")

class Appliance:
	def __init__(self, appliance):
		self.appliance = appliance
	
	def path(self):
		return zarfile_dir+self.appliance+".zarfile"

	def url(self):
		return "file:"+self.path()

	def __repr__(self):
		return self.appliance

class NullAppliance(Appliance):
	def __init__(self):
		Appliance.__init__(self, None)

	def path(self):
		return "/dev/null"

	def __repr__(self):
		return "null"

class Params:
	# the experimental configuration (input parameters)
	def __init__(self, from_appliance, to_appliance, compression_config):
		self.protocol = ""
		self.from_appliance = from_appliance
		self.to_appliance = to_appliance
		self.compression_config = compression_config

	def __repr__(self):
		return "%s %s->%s C(%s)" % (self.protocol, self.from_appliance, self.to_appliance, self.compression_config)

class Stats:
	# the experimental results (output data) for a given configuration
	def __init__(self, params, rx, tx):
		self.params = params
		self.params = params
		self.rx = rx
		self.tx = tx

	def __repr__(self):
		return "%-40s r %10s t %10s" % (self.params, self.rx, self.tx)

class Cmd:
	def __init__(self, args):
		self.args = args

class Shell:
	def __init__(self, cmd, stderrlog, **kv):
		stderrlog.write("Shell.cmd = %s\n" % (" ".join(cmd.args)))
		stderrlog.flush()
		proc = subprocess.Popen(cmd.args, stdout=subprocess.PIPE, stderr=stderrlog, **kv)
		self.stdout_data = proc.stdout.read()
		self.result = proc.wait()
	
class ShellFailedException(Exception):
	pass

def Do(cmd):
	s = Shell(cmd, stderrlog=sys.stderr)
	#print "s.result==",s.result
	if (s.result!=0):
		raise ShellFailedException()

class Server:
	def __init__(self, cmd, serverlog, **kv):
		serverlog.write("Server.cmd = %s\n" % (" ".join(cmd.args)))
		serverlog.flush()
		self.serverlog = serverlog
		self.serverlog.write("created from python\n")
		self.proc = subprocess.Popen(cmd.args, stdout=self.serverlog, stderr=self.serverlog, **kv)

	def terminate(self):
		self.proc.terminate()
		self.proc.wait()
		self.serverlog.write("terminated from python\n")
		self.serverlog.flush()

class ZftpServer(Server):
	def __init__(self, compression_config, serverlog):
		#print "cc: %s" % compression_config
		Server.__init__(self, Cmd([
			zftp_backend_path,
			"--origin-filesystem", "true",
			"--origin-reference", "true",
			"--listen-zftp", "tunid",
			"--listen-lookup", "tunid",
			"--index-dir", zftp_index_path,
			"--use-compression", compression_config.use_compression,
			"--zlib-compression-level", compression_config.zlib_compression_level,
			"--zlib-compression-strategy", compression_config.zlib_compression_strategy,
		]),
			serverlog = serverlog)
		self.compression_config = compression_config

	def get_compression_config(self):
		return self.compression_config

class MeasureZftp(Shell):
	def __init__(self, cmd, client_log, params):
		Shell.__init__(self, cmd, stderrlog=client_log)
		try:
			fields = self.stdout_data.split()
			rx = fields[2]
			tx = fields[5]
			self.stats = Stats(params, rx=rx, tx=tx)
		except:
			print "trouble decoding output '%s'" % self.stdout_data
			raise

class ZftpXfrBytes(MeasureZftp):
	def __init__(self, framework, params):
		MeasureZftp.__init__(self, Cmd([
			zftp_backend_path,
			"--origin-lookup", "tunid",
			"--origin-zftp", "tunid",
			"--no-show-payload",
			"--print-net-stats", "true",
			"--warm-url", params.from_appliance.url(),
			"--fetch-url", params.to_appliance.url(),
			"--use-compression", params.compression_config.use_compression,
			"--no-timeout"
			]),
			client_log=framework.get_client_log(),
			params=params)

	@classmethod
	def needs_zftp_server(self):
		return True

class RsyncXfrBytes(Shell):
	def __init__(self, framework, params):
		tmp1 = "/tmp/f1"
		tmp2 = "/tmp/f2"
		Do(Cmd(["rm", "-f", tmp1]))
		Do(Cmd(["rm", "-f", tmp2]))
		# the known file we're going to replace
		Do(Cmd(["cp", params.from_appliance.path(), tmp2]))
		# the new bits we want to transfer
		Do(Cmd(["cp", params.to_appliance.path(), tmp1]))
		zopt = ""
		if (params.compression_config.use_compression):
			zopt = "--compress-level=%s" % params.compression_config.zlib_compression_level
		Shell.__init__(self, Cmd(["rsync"]
			+[zopt]
			+["-v",
			"--no-whole-file",
			tmp1,
			tmp2]),
			stderrlog=framework.get_client_log())
		Do(Cmd(["rm", tmp1]))
		Do(Cmd(["rm", tmp2]))

		lines = self.stdout_data.split("\n")
		dataline = filter(lambda l: "received" in l, lines)[0]
		fields = dataline.split()
		# NB inverting sense of 'tx' and 'rx' to be from receiver's pov
		rx = fields[1]
		tx = fields[4]
		self.stats = Stats(params, rx=rx, tx=tx)

#		fields = self.stdout_data.split()
#		rx = fields[2]
#		tx = fields[5]
#		print "t %s r %s" % (tx,rx)

	@classmethod
	def needs_zftp_server(self):
		return False

class CompressionConfig:
	def __init__(self, use_compression, zlib_compression_level, zlib_compression_strategy):
		self.use_compression = str(use_compression).lower()
		self.zlib_compression_level = str(zlib_compression_level)
		self.zlib_compression_strategy = str(zlib_compression_strategy)

	def __repr__(self):
		if (self.use_compression):
			return "%s:%s" % (
				self.zlib_compression_level,
				self.zlib_compression_strategy[2])
		else:
			return "none"
		#return str(self._tuple())

	def __cmp__(self, other):
		return cmp(self._tuple(), other._tuple())

	def _tuple(self):
		return (self.use_compression, self.zlib_compression_level, self.zlib_compression_strategy)

rsync_compression_choices = [
	CompressionConfig(False, 0, "Z_DEFAULT_STRATEGY"),
	CompressionConfig(True, 1, "Z_DEFAULT_STRATEGY"),
	CompressionConfig(True, 9, "Z_DEFAULT_STRATEGY"),
	]

zftp_compression_choices = [
	CompressionConfig(False, 0, "Z_DEFAULT_STRATEGY"),
	CompressionConfig(True, 1, "Z_DEFAULT_STRATEGY"),
	CompressionConfig(True, 9, "Z_DEFAULT_STRATEGY"),
	CompressionConfig(True, 1, "Z_RLE"),
	CompressionConfig(True, 9, "Z_RLE"),
	]

appliances = [NullAppliance()] + map(Appliance, [
#	"microsoft-hot",
#	"abiword",
#	"gimp",
#	"gnumeric",
#	"inkscape",
	"marble",
	"zhyperoid"
#	"gnucash",
	])

class Experiment:
	def __init__(self, exp_class, params):
		self.exp_class = exp_class
		self.params = params
	
	def run(self, framework):
		return (self.exp_class)(framework, self.params)

	def __repr__(self):
		return "%s:%s" % (self.exp_class.__name__, self.params)

	def needs_zftp_server(self):
		return self.exp_class.needs_zftp_server()

def full_matrix():
	exps = []
	for from_idx in range(len(appliances)):
		for to_idx in range(len(appliances)):
			for cc in rsync_compression_choices:
				exps.append(Experiment(
					RsyncXfrBytes,
					Params(appliances[from_idx], appliances[to_idx], cc)))
			for cc in zftp_compression_choices:
				exps.append(Experiment(
					ZftpXfrBytes,
					Params(appliances[from_idx], appliances[to_idx], cc)))
	return exps

class Framework:
	def __init__(self):
		self._serverlog = open("zftp_server.log", "w+")
		
		exps = full_matrix()
		self.last_zftp_zerver = None
		for expi in range(16, len(exps)):
			exp = exps[expi]
			print "starting [%d] %s" % (expi, exp)
			self.setup(exp)
			result = exp.run(self)
			print result.stats
		self.shutdown()

	def get_server_log(self):
		self._serverlog.write("/--- server starts\n")
		self._serverlog.flush()
		return self._serverlog

	def get_client_log(self):
		self._serverlog.write("/--- client starts\n")
		self._serverlog.flush()
		return self._serverlog

	def setup(self, exp):
		if (not exp.needs_zftp_server()):
			return
		if (self.last_zftp_zerver != None):
			if (self.last_zftp_zerver.get_compression_config()
				== exp.params.compression_config):
				# oh, it's already the right server
				return
			else:
				# kill it to make room for a new one.
				self.last_zftp_zerver.terminate()

		self.last_zftp_zerver = ZftpServer(
			exp.params.compression_config, self.get_server_log())

	def shutdown(self):
		if (self.last_zftp_zerver != None):
			self.last_zftp_zerver.terminate()
			self._serverlog.write("\---- server ends\n")
			self._serverlog.flush()

	def get_zftp_server(self):
		return self.last_zftp_zerver

#	zf = ZftpXfrBytes(zs, NullAppliance(), Appliance("zhyperoid"))
Framework()
