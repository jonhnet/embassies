from StartTimeCore import *

class GimpPerf(MidoriCommon):
	def __init__(self, monitor_type, elf_loader_config = None):
		MidoriCommon.__init__(self, "GimpPerf")
		self.monitor_type = monitor_type
		if (elf_loader_config == None):
			self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/elf_loader/build/elf_loader.gimpperf.signed"
		else:
			self.elf_loader_config = elf_loader_config

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_stderr"

	def last_perf_string(self): return "end_rotate"

	def requested_spans(self):
		return [
			Span("99_total", "start_rotate", "end_rotate"),
			]

class EvalGimpPerf(EvaluateMidori):
	def __init__(self, monitor_type):
		EvaluateMidori.__init__(self, "EvalGimpPerf")
		self.monitor_type = monitor_type

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return GimpPerf(self.monitor_type)
		
class TopGimpPerf:
	def __init__(self, monitor_type):
		steps = [
			Monitor(monitor_type),
			ZftpBackend(),
			KeyVal(monitor_type),
			ZftpZoog(monitor_type),
			EvalGimpPerf(monitor_type)
		]
		execute_steps(steps)

class GimpPerfPosix(MidoriCommon):
	def __init__(self):
		MidoriCommon.__init__(self, "GimpPerfPosix")

	def cmd(self):
		return CmdSpec([
			ZOOG_ROOT + "toolchains/linux_elf/lib_links/gimpperf-pie",
				"--no-data",
				"--no-fonts",
				"-i"
			])

	def debug_perf_file(self): return "stderr"

	def last_perf_string(self): return "end_rotate"

	def requested_spans(self):
		return [
			Span("99_total", "start_rotate", "end_rotate"),
			]

class EvalGimpPerfPosix(EvaluateMidori):
	def __init__(self):
		EvaluateMidori.__init__(self, "EvalGimpPerfPosix")
		self.monitor_type = monitor_type

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return GimpPerfPosix()

class TopGimpPerfPosix:
	def __init__(self, vendor):
		sudouser = "net-restricted"
#		sudouser = None
		steps = [ EvalGimpPerfPosix() ]
		execute_steps(steps)

