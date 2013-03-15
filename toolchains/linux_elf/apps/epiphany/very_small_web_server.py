#!/usr/bin/python

import BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
BaseHTTPServer.test(SimpleHTTPRequestHandler, BaseHTTPServer.HTTPServer)
