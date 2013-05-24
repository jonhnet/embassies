#!/usr/bin/python

import operator
import numpy
import glob

class Datum:
	def __init__(self, bench, delta, mean, median, stddev):
		self.bench = bench
		self.delta = delta
		self.mean = mean
		self.median = median
		self.stddev = stddev

class Stats:
	def __init__(self, text):
		self.data = {}
		self.benches = set()
		self.deltas = set()
		for row in text.split("\n"):
			if (row==""): continue
			fields = row.split(",")
			desc = fields[0]
			(bench,delta) = desc.split(":")
			data = map(lambda s: float(s)*1000, fields[1:])
			self.benches.add(bench)
			self.deltas.add(delta)
			d = Datum(bench,
				delta,
				numpy.average(data),
				numpy.median(data),
				numpy.std(data))
			self.data[(bench,delta)] = d

	def display(self):
		benchlist = list(self.benches)
		benchlist.sort()
		deltalist = list(self.deltas)
		deltalist.sort()
		rows = []
		NULL = ''
		title1 = reduce(operator.add, map(lambda b: [b,NULL,NULL], benchlist), [NULL])
		rows.append(title1)
		subtitle = reduce(operator.add, map(lambda b: ['mean','median','sd'], benchlist), [''])
		rows.append(subtitle)
		for delta in deltalist:
			def fetch_data(bench):
				try:
					d = self.data[(bench,delta)]
					return list(map(lambda t: "%.1f"%t, [d.mean,d.median,d.stddev]))
				except KeyError:
					return [NULL,NULL,NULL]
			datarow = reduce(operator.add, map(fetch_data, benchlist), [str(delta)])
			rows.append(datarow)

		ofp = open("exp.csv", "w")
		for row in rows:
			ofp.write(",".join(map(lambda s: "%s"%s, row))+"\n")
		ofp.close()

def main():
	text = ""
	for path in glob.glob("*Eval*.runtimes"):
		text += open(path).read()
	Stats(text).display()

main()
