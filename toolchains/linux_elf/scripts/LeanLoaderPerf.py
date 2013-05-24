from StartTimeCore import *

class LeanLoader(MidoriCommon):
	def __init__(self, monitor_type, elf_loader_config = None):
		MidoriCommon.__init__(self, "LeanLoader")
		self.monitor_type = monitor_type
		if (elf_loader_config == None):
			self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/leanloader/build/leanloader.signed"
		else:
			self.elf_loader_config = elf_loader_config

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_perf"

	def last_perf_string(self):
		return "done"

	def requested_spans(self):
		return [
			Span("99_total", "start", "done"),
			]

class YEvalLeanLoader(EvaluateMidori):
	def __init__(self, monitor_type):
		EvaluateMidori.__init__(self, "YEvalLeanLoader")
		self.monitor_type = monitor_type

	def cool_time(self):
		return 1

	def prep(self): return []

	def experiment(self): return LeanLoader(self.monitor_type)
		
class TopLeanLoader:
	def __init__(self, monitor_type):
		steps = [
			Monitor(monitor_type),
			YEvalLeanLoader(monitor_type)
		]
		execute_steps(steps)

