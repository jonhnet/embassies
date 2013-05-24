#!/usr/bin/python

import sys

class Event:
	def __init__(self, name, time):
		self.name = name
		self.time = time

class Parse:
	def __init__(self, path):
		translate_mode = False
		if (len(sys.argv)>1 and sys.argv[1]=="-t"):
			translate_mode = True

		fp = open(path)
		eventlog = []
		while (True):
			line = fp.readline()
			if (line==""): break
			line = line.strip()
			(label, timehex, event_name) = line.split(' ', 2)
			if (translate_mode):
				event_name = "%s.%s" % (len(eventlog), event_name)
			eventlog.append(Event(event_name, int(timehex, 16)))

#		pairs = events.items()
#		def byvalue(t0,t1):
#			return cmp(t0[1], t1[1])
#		pairs.sort(byvalue)

		start_event = eventlog[0]
		for event in eventlog:
			if (event.name==start_event.name):
				start_event = event
				print
			diff = (event.time - start_event.time)/1000000000.0
			print "%f: %s" % (diff, event.name)

try:
	path = sys.argv[1]
except:
	path = "debug_perf"
Parse(path)
