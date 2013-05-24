#!/usr/bin/python
import signed_binary_magic

import sys
import struct

certfile = sys.argv[1]
rawfile = sys.argv[2]
signedfile = sys.argv[3]


ifp = open(certfile, "rb")
certbytes = ifp.read()
ifp.close()

ifp = open(rawfile, "rb")
rawbytes = ifp.read()
ifp.close()

ofp = open(signedfile, "wb")
ofp.write(struct.pack("!III", signed_binary_magic.big_endian_magic, len(certbytes), len(rawbytes)))
ofp.write(certbytes)
ofp.write(rawbytes)
ofp.close()
