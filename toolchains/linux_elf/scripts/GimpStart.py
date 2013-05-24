from StartTimeCore import *

class GimpStart(MidoriCommon):
	def __init__(self, monitor_type, elf_loader_config = None):
		MidoriCommon.__init__(self, "GimpStart")
		self.monitor_type = monitor_type
		if (elf_loader_config == None):
			self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/elf_loader/build/elf_loader.gimpperf.signed"
		else:
			self.elf_loader_config = elf_loader_config

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_perf"

	def last_perf_string(self): return "ready"

	def requested_spans(self):
		return [
			Span("11_fetch", "start", "mac_lookup_start"),
			Span("21_hot_mac_verify", "mac_verify_start", "mac_verify_end"),
			Span("99_total", "start", "ready"),
			]

class EvalGimpStart(EvaluateMidori):
	def __init__(self, monitor_type):
		EvaluateMidori.__init__(self, "EvalGimpStart")
		self.monitor_type = monitor_type

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return GimpStart(self.monitor_type)
		
class TopGimpStart:
	def __init__(self, monitor_type):
		steps = [
			Monitor(monitor_type),
			ZftpBackend(),
			KeyVal(monitor_type),
			ZftpZoog(monitor_type),
			GimpStart(monitor_type),	# warm the cache for the start measurement
			EvalGimpStart(monitor_type)
		]
		execute_steps(steps)

class GimpStartPosix(MidoriCommon):
	def __init__(self):
		MidoriCommon.__init__(self, "GimpStartPosix")

	def cmd(self):
		return CmdSpec([
			ZOOG_ROOT + "toolchains/linux_elf/posix_perf_measure/build/posix_perf_measure",
			ZOOG_ROOT + "toolchains/linux_elf/lib_links/gimpperf-pie",
#				"--no-data",
#				"--no-fonts",
#				"-i"
			])

	def debug_perf_file(self): return "debug_perf_posix"

	def last_perf_string(self): return "ready"

	def requested_spans(self):
		return [
			Span("99_total", "start", "ready"),
			]

class EvalGimpStartPosix(EvaluateMidori):
	def __init__(self):
		EvaluateMidori.__init__(self, "EvalGimpStartPosix")

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return GimpStartPosix()

class TopGimpStartPosix:
	def __init__(self, vendor):
		steps = [ EvalGimpStartPosix() ]
		execute_steps(steps)

