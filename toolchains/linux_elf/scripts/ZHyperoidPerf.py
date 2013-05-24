from StartTimeCore import *

class Zhyperoid(MidoriCommon):
	def __init__(self, monitor_type, elf_loader_config = None):
		MidoriCommon.__init__(self, "Zhyperoid")
		self.monitor_type = monitor_type
		if (elf_loader_config == None):
			self.elf_loader_config = ZOOG_ROOT + "toolchains/linux_elf/elf_loader/build/elf_loader.zhyperoid.signed"
		else:
			self.elf_loader_config = elf_loader_config

	def cmd(self):
		return self.monitor_type.pal_cmd(self.elf_loader_config)

	def debug_perf_file(self): return "debug_perf"

	def last_perf_string(self):
		return "first_paint"

	def requested_spans(self):
		return [
			Span("10_zoog_start", "start", "request_zarfile"),
			Span("11_fetch", "request_zarfile", "mac_lookup_start"),
			Span("20_hot_mac_lookup", "mac_lookup_start", "mac_verify_start"),
			Span("21_hot_mac_verify", "mac_verify_start", "mac_verify_end"),
			Span("22_hot_app_boot", "mac_verify_end", self.last_perf_string()),
			Span("99_total", "start", "first_paint"),
			]

class XEvalZhyperoid(EvaluateMidori):
	def __init__(self, monitor_type):
		EvaluateMidori.__init__(self, "XEvalZhyperoid")
		self.monitor_type = monitor_type

#	def cool_time(self):	return 2

	def prep(self): return []

	def experiment(self): return Zhyperoid(self.monitor_type)

class TopZhyperoid:
	def __init__(self, monitor_type):
		steps = [
			Monitor(monitor_type),
			ZftpBackend(),
			KeyVal(monitor_type),
			ZftpZoog(monitor_type),
#			Zhyperoid(monitor_type),
			XEvalZhyperoid(monitor_type)
		]
		execute_steps(steps)

