#!/usr/bin/python

import sys
import time
import stat
import os
import struct

class ChannelLog:
	def __init__(self, filename):
		self.filename = filename
		self.open_outputs = {}
		self.generation_number = None
		self.reset()

	def reset(self):
		self.offset = 4	# skip generation number

		for channel_name in self.open_outputs.keys():
			sys.stderr.write("clearing %s\n" % channel_name)
			ofp = self.open_outputs[channel_name]
			ofp.seek(0)
			ofp.truncate(0)

	def output_file(self, channel_name):
		if (channel_name in self.open_outputs):
			return self.open_outputs[channel_name]
		ofp = open("debug_"+channel_name, "a")
		self.open_outputs[channel_name] = ofp
		return ofp

	def unpack_one(self):
		self.fp.seek(self.offset);
		try:
			record_length = struct.unpack("@I", self.fp.read(4))[0]
		except:
			record_length = 0
		if (record_length==0):
			return False
#		sys.stderr.write("unpack_one at offset %d, record_length = %d\n" % (self.offset, record_length))

		name_length = struct.unpack("@I", self.fp.read(4))[0]
		data_length = struct.unpack("@I", self.fp.read(4))[0]
		channel_name = self.fp.read(name_length)

		ofp = self.output_file(channel_name)

		left = data_length
		while (left>0):
			amount = left
			if (amount>65536):
				amount = 65536 
			data = self.fp.read(amount)
			ofp.write(data)
			left -= amount
		if (data_length > 1<<20):
			sys.stderr.write("emitted %dMB to %s\n" %
				(data_length>>20, channel_name))

		pad_length = record_length - 12 - name_length - data_length
		#sys.stderr.write("record_length %d pad_length %d\n" % (record_length, pad_length))
		assert(pad_length>=0 and pad_length<4)
		self.fp.read(pad_length)
#		if (data_length < 100):
#			sys.stderr.write("%s: %s" % (name, data))
#		else:
#			sys.stderr.write("%s: data[%d]\n" % (name, data_length))

		ofp.flush()
		self.offset = self.fp.tell()

		return True

	def unpack_many(self):
		self.fp = open(self.filename, "a+")

		self.fp.seek(0)
		generation_number = struct.unpack("@I", self.fp.read(4))[0]
		if (self.generation_number != generation_number):
			sys.stderr.write("log file rewritten; starting over; now %d\n" % generation_number)
			self.generation_number = generation_number
			self.reset()

		while (True):
			rc = self.unpack_one()
			if (not rc):
				break
		self.fp.close()

	def poll(self):
		while (True):
			self.unpack_many()
			time.sleep(0.1)
	
cl = ChannelLog("qemu.log")
cl.poll()
